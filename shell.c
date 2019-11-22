#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAXSIZE 255

int get_command(char* cmd, char **argv);
int inner_command(char **argv);
void do_command(char **argv);
void do_pipe(int pp, char **argv);
void do_redirect(int flag, int dp, char **argv);
int check_command(int argc, char **argv);


int main(){

	char prompt[] = "JJ's SHELL ";
	char *dir;
	int inner_flag;
	int i;

	printf("--------------------------------------\n");
	printf("myeongju , hyejin \n");
	printf(" [ 기능 ]\n");
	printf(" 쉘 명령어 : cd, exit\n");
	printf(" 외부 명령어 : pipe(|), redirection(<,<<,>,>>)\n");
	printf("--------------------------------------\n");

	while(1){
		char command[MAXSIZE] = {'\0',};
		char **argv;
		int argc=0;

		dir = (char *)malloc(64*sizeof(char));
		getcwd(dir, MAXSIZE);

		//포인터 벼수 메모리 동적 할당
		argv = (char **)malloc(32*sizeof(char *));
		for(i = 0; i<32; i++){
			argv[i] = (char *)malloc(64*sizeof(char));
		}

		//입력한 명령어 읽기
		write(STDOUT_FILENO, prompt, sizeof(prompt));
		write(STDOUT_FILENO, strrchr(dir,'/')+1, strlen(strrchr(dir,'/')));
		write(STDOUT_FILENO, "> ", 3);
		read(STDIN_FILENO, command, MAXSIZE);

		argc = get_command(command, argv);

		inner_flag = inner_command(argv);
		if(inner_flag==0)
			check_command(argc, argv);
	}
	return 0;
}

//명령어 분류
int get_command(char* cmd, char **argv){
	int off_s = 0;
	int off_e = 0;
	int argc = 0;
	char temp;

	while(cmd[off_e] != '\n'){
		temp = cmd[off_e];
		if(temp==' '){
			strncpy(argv[argc], cmd+off_s, off_e-off_s);
			argc++;
			off_s = off_e+1;
		}
		off_e++;
	}

	strncpy(argv[argc], cmd+off_s, off_e-off_s);
	argc++;
	argv[argc]=(char *)0;
	return argc;
}

//내부 명령어 처리
int inner_command(char **argv){
	
	if(strcmp(argv[0], "exit") == 0){
		printf("종료합니다.\n");
		exit(1);
	}
	if(strcmp(argv[0], "cd") == 0){
		chdir(argv[1]);
		return 1;
	}

	return 0;
}

//외부 명령어 처리
void do_command(char **argv){

	pid_t pid;
	int exit_status;

	if((pid = fork()) == -1 ){
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

//파이프 명령어 처리
void do_pipe(int pp, char **argv){

	char **fw_argv;
	char **bw_argv;

	int i, j =0;
	pid_t pid;
	int fd[2];

	fw_argv = (char **)malloc(32*sizeof(char *));
	for(i = 0; i < 32; i++)
		fw_argv[i] = (char *)malloc(64*sizeof(char));


	bw_argv= (char **)malloc(32*sizeof(char *));
	for(i =0; i<32; i++)
		bw_argv[i] = (char *)malloc(64*sizeof(char));

	for(i =0; i < pp; i++){
		strcpy(fw_argv[i], argv[i]);
	}


	fw_argv[pp] = (char *)0;
	for(i = pp+1; argv[i]!=NULL; i++){
		strcpy(bw_argv[j], argv[i]);
		j++;
	}

	bw_argv[j] = (char *)0;

	pipe(fd);

	if((pid =fork()) == -1){
		printf("fork() error!\n");
	}else if(pid == 0){
		if(fork() == 0){
			close(fd[1]);
			dup2(fd[0], 0);
			close(fd[0]);

			if(execvp(bw_argv[0], bw_argv) == -1){
				if(strlen(bw_argv[0])!=0){
					printf("명령을 찾을 수 없습니다. %s\n", bw_argv[0]);
				}
			exit(1);
			}
		}
		else{
			close(fd[0]);
			dup2(fd[1], 1);
			close(fd[1]);
			if(execvp(fw_argv[0], fw_argv)==-1){
				if(strlen(fw_argv[0])!=0){
						printf("명령을 찾을 수 없습니다. %s\n", fw_argv[0]);
				}
			exit(1);
			}
		}
	}
	else{
		wait(1);
	}
}


//리다이렉션 명령어 처리
void do_redirect(int flag, int dp, char **argv){

	char **fw_argv;
	char **bw_argv;
	int i, j =0;
	int fd;
	int exit_status;
	pid_t pid;

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
				printf("fork() 에러");exit(1); 
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
				printf("fork() 에러");exit(1);
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
				printf("fork() 에러");exit(1); 
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

		case 4:
			if((pid=fork())==-1){ 
				printf("fork() 에러");exit(1); 
			}else if(pid == 0){
				fd=open(bw_argv[0], O_RDONLY);
				dup2(fd,STDIN_FILENO);
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

//파이프 체크
int check_command(int argc, char **argv){

	int i=0; int find[2] = {0, };

	for(i = 0; i < argc; i++){
		if(strcmp(argv[i],">")==0){
			find[0] = 1;
			find[1] = i;
			break;
		}
		else if(strcmp(argv[i], ">>") == 0){
			find[0] = 2;
			find[1] = i;
			break;
		}

		else if(strcmp(argv[i], "<")==0){
			find[0] = 3;
			find[1] = i;
			break;
		}
		else if(strcmp(argv[i], "<<")==0){
			find[0] = 4;
			find[1] = i;
			break;
		}
		else if(strcmp(argv[i], "|")==0){
			find[0] = 5;
			find[1] = i;
			break;
		}
		else{
			find[0] = 0;
			find[1] = i;
		}
	}

	switch(find[0]){
		case 0:
			do_command(argv);
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
			do_redirect(find[0], find[1], argv);
			break;
		case 5:
			do_pipe(find[1], argv);
			break;
		default:
			break;
	}
}


