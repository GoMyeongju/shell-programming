#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>

#define MAXSIZE 255

int get_comm(char* cmd, char **argv);
int inner_comm(char **argv);
void do_comm(char **argv);
void do_pipe(int pp, char **argv);
void do_redirect(int flag, int dp, char **argv);
int check_comm(int argc, char **argv);
void do_background(int p, char **argv);
void do_signal(int signum);

pid_t pid; //자식 프로세스 저장 전역변수 

int main(){

	//쉘 이름
	char prompt[] = "JJ's SHELL ";
	char *dir;
	int inner_flag;
	int i;

	//초기 출력 화면
	printf("--------------------------------------\n");
	printf("Hello, Welecome to JJ's Shell!\n");
	printf("made by myeongju , hyejin \n");
	printf("--------------------------------------\n");
	//signal(SIGINT, do_signal);
	//signal(SIGTSTP, do_signal);

	while(1){
		char command[MAXSIZE] = {'\0',};
		char **argv;
		int argc=0;


		dir = (char *)malloc(64*sizeof(char));
		getcwd(dir, MAXSIZE);

		//포인터 변수 메모리 동적 할당
		argv = (char **)malloc(32*sizeof(char *));
		for(i = 0; i<32; i++){
			argv[i] = (char *)malloc(64*sizeof(char));
		}
		signal(SIGINT, do_signal);
		signal(SIGTSTP, do_signal);



		//printf("%d, %d\n", getpid(), getppid());
		//쉘 프롬프트 출력 및 입력한 명령어 읽어옴.
		write(STDOUT_FILENO, prompt, sizeof(prompt));
		write(STDOUT_FILENO, strrchr(dir,'/')+1, strlen(strrchr(dir,'/')));
		write(STDOUT_FILENO, "> ", 3);
		read(STDIN_FILENO, command, MAXSIZE);



		//명령어 토큰 분류
		argc = get_comm(command, argv);

		//내부 명령어일 경우 처리
		inner_flag = inner_comm(argv);

		//외부 명령어(일반, 파이브, 백그라운드, 리다이렉션)일 경우 처리
		if(inner_flag==0)
			check_comm(argc, argv);
	}
	
	return 0;
}

//시그널 처리
void do_signal(int signum){

	int stat;
	//int child = getpid();

	//프로세스 구분(자식인지 아닌지)
	if(pid  != getppid()){
		switch(signum){
			case SIGINT:
				printf("Ctrl + C\n");
				break;
			case SIGTSTP:
				printf("Ctrl + Z\n");
				pid = wait(&stat);
				//kill(kill, SIGCHLD);
				break;
		}
	}
	
}


//명령어 받아와 각각의 토큰으로 나눠 분석
int get_comm(char* cmd, char **argv){
	int comm_s = 0;
	int comm_e = 0;
	int argc = 0;
	char temp;

//	pid_t pid; //자식 프로세스 저장 
	while(cmd[comm_e] != '\n'){
		temp = cmd[comm_e];
		if(temp==' '){
			strncpy(argv[argc], cmd+comm_s, comm_e-comm_s);
			argc++;
			comm_s = comm_e+1;
		}
		comm_e++;
	}

	strncpy(argv[argc], cmd+comm_s, comm_e-comm_s);
	argc++;
	argv[argc]=(char *)0;
	return argc;
}

//내부 명령어 처리
int inner_comm(char **argv){
	
	//exit 
	if(strcmp(argv[0], "exit") == 0){
		printf("JJ's Shell을 종료합니다.\n");
		exit(1);
	}

	//cd
	if(strcmp(argv[0], "cd") == 0){
		chdir(argv[1]);
		return 1;
	}

	return 0;
}

//일반 외부 명령어 처리(책에 있는 코드 참조)
void do_comm(char **argv){

	int exit_status;
//	pid_t pid;

	if((pid =fork()) == -1 ){
		printf("fork() error!\n");
	}
	else if(pid ==0){
		if(execvp(argv[0], argv) == -1)
			if(strlen(argv[0]) != 0)
				printf("명령을 찾을 수 없습니다. %s\n", argv[0]);
		exit(1);
	}
	else{
		wait(&exit_status);
		}
}

//내가 정의한 외부 명령어 분류
int check_comm(int argc, char **argv){

	int i=0; int find[2] = {0, };

	for(i = 0; i < argc; i++){
		//리다이렉션
		if(strcmp(argv[i],">") == 0){
			find[0] = 1;
			find[1] = i;
			break;
		}
		else if(strcmp(argv[i], ">>") == 0){
			find[0] = 2;
			find[1] = i;
			break;
		}

		else if(strcmp(argv[i], "<") == 0){
			find[0] = 3;
			find[1] = i;
			break;
		}
		//백그라운드
		else if(strcmp(argv[i], "&") == 0){
			find[0] = 4;
			find[1] = i;
			break;
		}
		//파이프
		else if(strcmp(argv[i], "|") == 0){
			find[0] = 5;
			find[1] = i;
			break;
		}//일반  내부 명령어
		else{
			find[0] = 0;
			find[1] = i;
		}
	}

	switch(find[0]){
		case 0:
			do_comm(argv);
			break;
		case 1:
			do_redirect(find[0], find[1], argv);
			break;
		case 2:
			do_redirect(find[0], find[1], argv);
			break;
		case 3:
			do_redirect(find[0], find[1], argv);
			break;
		case 4:
			do_background(find[1], argv);
			break;
		case 5:
			do_pipe(find[1], argv);
			break;
		default:
			break;
	}
}


//파이프 명령어 처리
void do_pipe(int pp, char **argv){

	char **fc_argv;
	char **bc_argv;

	int i, j =0;
	int fd[2];

//	pid_t pid; //자식 프로세스 저장 
	fc_argv = (char **)malloc(32*sizeof(char *));
	for(i = 0; i < 32; i++)
		fc_argv[i] = (char *)malloc(64*sizeof(char));


	bc_argv= (char **)malloc(32*sizeof(char *));
	for(i =0; i<32; i++)
		bc_argv[i] = (char *)malloc(64*sizeof(char));

	for(i =0; i < pp; i++){
		strcpy(fc_argv[i], argv[i]);
	}


	fc_argv[pp] = (char *)0;
	for(i = pp+1; argv[i]!=NULL; i++){
		strcpy(bc_argv[j], argv[i]);
		j++;
	}

	bc_argv[j] = (char *)0;

	pipe(fd);

	if((pid =fork()) == -1){
		printf("fork() error!\n");
	}else if(pid == 0){
		if(fork() == 0){
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);

			if(execvp(bc_argv[0], bc_argv) == -1){
				if(strlen(bc_argv[0])!=0){
					printf("명령을 찾을 수 없습니다. %s\n", bc_argv[0]);
				}
			exit(1);
			}
		}
		else{
			close(fd[0]);
			dup2(fd[1], 1);
			close(fd[1]);
			if(execvp(fc_argv[0], fc_argv)==-1){
				if(strlen(fc_argv[0])!=0){
						printf("명령을 찾을 수 없습니다. %s\n", fc_argv[0]);
				}
			exit(1);
			}
		}
	}
	else{
		wait(1);
	}
}

//백그라운드 명령어 처리
void do_background(int p, char **argv){


//	pid_t pid; //자식 프로세스 저장

	int exit_status; //자식 프로세스 종료 시 상태 저장  
	int i;
	char **fc_argv;

	fc_argv = (char **)malloc(32*sizeof(char *));
	for(i = 0; i < 32; i++)
		fc_argv[i] = (char *)malloc(64*sizeof(char *));

	for(i = 0; i < p; i++){
		strcpy(fc_argv[i], argv[i]);
	}

	fc_argv[p] = (char *)0;
	if((pid =fork()) == -1){
		printf("fork() error!\n");
	}else if(pid == 0){
		if(fork() == 0){
			printf("자식 프로세스 ID : %d\n", getpid());
			if(execvp(fc_argv[0], fc_argv)==-1){
				if(strlen(fc_argv[0]) != 0){
					printf("명령을 찾을 수 없습니다. %s\n", fc_argv[0]);
				}
			}	
			exit(1);
		}else{
			exit(1);
		}
	}else{
	
		wait(&exit_status);
	}

}

//리다이렉션 명령어 처리
void do_redirect(int flag, int dp, char **argv){

	char **fw_argv;
	char **bw_argv;
	int i, j =0;
	int fd;
	int exit_status;
//	pid_t pid; //자식 프로세스 저장

	fw_argv=(char **)malloc(32*sizeof(char *));
	for(i=0;i<32;i++)
		fw_argv[i]=(char *)malloc(64*sizeof(char));

	bw_argv=(char **)malloc(32*sizeof(char *));
	for(i=0;i<32;i++)
		bw_argv[i]=(char *)malloc(64*sizeof(char));

	for(i = 0; i <dp; i++){
		strcpy(fw_argv[i], argv[i]);
	}

	fw_argv[dp] = (char *)0;
	strcpy(bw_argv[0],argv[dp+1]);
	bw_argv[dp+2]=(char *)0;

	switch(flag){
		case 1:
			if((pid=fork())==-1){ 
				printf("fork() 에러\n");exit(1); 
			}else if(pid==0){
				close(fd);
				fd=open(bw_argv[0], O_RDWR | O_CREAT | O_TRUNC, 0644);
				dup2(fd,STDOUT_FILENO);
				write(fd,STDOUT_FILENO,sizeof(STDOUT_FILENO));
				close(fd);
				if(execvp(fw_argv[0], fw_argv)==-1)
					if(strlen(fw_argv[0])!=0)
						printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
				exit(1);
			}else{
				wait(&exit_status);
			}
			break;

		case 2:
			if((pid=fork())==-1){ 
				printf("fork() 에러\n");exit(1);
			}else if(pid ==0){
				close(fd);
				fd = open(bw_argv[0], O_RDWR | O_CREAT | O_APPEND, 0644);
				dup2(fd,STDOUT_FILENO);
				write(fd,STDOUT_FILENO,sizeof(STDOUT_FILENO));
				close(fd);
				if(execvp(fw_argv[0], fw_argv)==-1)
					if(strlen(fw_argv[0])!=0)
						printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
				exit(1);
			}
			else{
				wait(&exit_status);
			}
			break;

		case 3:
			if((pid=fork())==-1){ 
				printf("fork() 에러\n");exit(1); 
			}else if(pid==0){
				fd=open(bw_argv[0], O_RDONLY);
				dup2(fd, STDIN_FILENO);
				close(fd);
				if(execvp(fw_argv[0], fw_argv)==-1)
					if(strlen(fw_argv[0])!=0)
						printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
				exit(1);
			}else{
				wait(&exit_status);
			}
			break;

		default:
			break;
	}
}


