#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<readline/readline.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAXCOM 512 
#define MAXLIST 100 
#define qcap 20

//This function accepts unpit from user starting from $
//char *path[2];
char path[MAXLIST][255];
int pathnum;
char Alias1[MAXLIST][255],Alias2[MAXLIST][255];
int Aliasnum;
char q[21][255],qsize,front,rear;

int T(char* str) //transfer char* into int
{
	int i=0,ans=0;
	while(str[i]!='\0')
	{
		ans*=10;
		ans+=str[i++]-'0';
	}
	return ans;
}

// handler for SIGTSTP
volatile sig_atomic_t sigtstp_sent;
void sigtstp_handler(int sig) {
	// setting sigtstp_sent
	sigtstp_sent = 1;
	// re-registering SIGTSTP handler
  	signal(SIGTSTP, sigtstp_handler);
}

//function to check if queue is full
int isFull(){
	return qsize == qcap;
}

//function to check if queue is empty
int isEmpty(){
	return qsize == 0;
}

//function to enqueue
void Enqueue(char* string_element){
	char* check;
	strncpy(check,string_element,9);
	check[9]='\0';
	// printf("%s\n",check);
	if(strcmp(check,"myhistory")==0)return;
	q[rear][0] = '\0';
	strcpy(q[rear],string_element);
	if(isFull()) front=(front+1)%qcap,qsize--;
	rear=(rear+1)%qcap;
	qsize++;
}

//make a print function for queue
void printHist(){
	printf("History Of Commands:\n");
	int i = front;
	if(isFull())printf("%s\n", q[i]),i=(i+1)%qcap;;
	for(; i != rear; i = (i+1)%qcap){
		printf("%s\n", q[i]);
	}
}

//signal handler for parent
void sig_handler_p(int signum){
	//ignores signals c, z
	signal(signum, SIG_IGN);
}

//signal handler for child
void sig_handler_c(int signum){
	printf("Child sig is working.\n");
	int pid = getpid();
	switch(signum){
		case SIGINT:
      		printf("[caught SIGINT]\n");
      		kill(pid, SIGINT);
     		break;
    	case SIGTSTP:	//ctrl z
      		printf("[caught SIGTSTP\n");
      		signal(SIGTSTP, SIG_DFL);
      		break;
    	default:
      		break; 
	}
}

//make a clean up function
//Implemented by Suleiman
int lineInput(char* str)
{
	char* buf;

	buf = readline(" $ ");
	if (strlen(buf) != 0) {
		strcpy(str, buf);
		return 0;
	} else {
		return 1;
	}
}


//this function finds and prints current directory of the executing shell
void pwdFunc()
{
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));
	printf("\n%s", cwd);
}

void pathinit()
{
	char* tmp = getenv("PATH");
	strcpy(path[0],tmp);
	pathnum = 1;
	// printf("%s\n",path[0]);
	int i=0,j=0;
	while(i++<strlen(path[0]))
	{
		if(path[0][i]==":"){
			path[pathnum][j]='\0';
			j=0;
			pathnum++;
		}
		else path[pathnum][j++] = path[0][i];
	}
	return;
}
void getpath()
{
	printf("PATH:\n%s",path[1]);
	for(int i=2;i<=pathnum;i++)
	{
		if(path[i][0]!='\0')
		printf(":%s",path[i]);
	}
	printf("\n");
}

//Implemented by Suleiman
//this function executes shell commands with their agruments using execvp
void execCommands(char** parsed)
{
	pid_t pid = fork(); 
	if (pid == 0) {
		//setpgid(0,0);	//gives unique pgid to the child
		signal(SIGINT, sig_handler_c);
		signal(SIGTSTP, sig_handler_c);
		//tcsetpgrp(0, getpgrp());    //sets child to be part of the foreground process
        if (execvp(parsed[0], parsed) < 0) {// parsed[0] is cmd parsed[1..] are args
            fprintf(stderr, "\nExecution Error: Command not found.");
        }
        exit(0);
    } else  if (pid == -1) {
        printf("\nFork Error");
        return;
    } else {//parent waits
		//setpgid(pid, pid);
		//tcsetpgrp(0, pid);
        wait(NULL); 
        return;
    }
}

//Implemented by Suleiman
//exec Commands piped 2
void execCmdPipes2(char **parsed, char **parsedPipe){
	int status;
	int i;
	int fd1[2];
	pipe(fd1);	//creating pipe
	int p1 = fork();
	if (p1 == 0)//child process
	{	
		setpgid(0,0);	//gives unique pgid to the child
		//tcsetpgrp(0, getpgrp());    //sets child to be part of the foreground process
		//executing first command before |
		dup2(fd1[1],1);
	  	close(fd1[0]);
		close(fd1[1]);
		execvp(parsed[0], parsed);
 	}
  	else if(p1 > 0)// parent 
    {
		//setpgid(p1, p1);
		//tcsetpgrp(0, p1);
		//executing second command after |
		int p2 = fork();
		if (p2 == 0)	//child process
		{
			setpgid(0,0);	//gives unique pgid to the child
			//tcsetpgrp(0, getpgrp());    //sets child to be part of the foreground process
			
	  		dup2(fd1[0], 0);
	  		close(fd1[0]);
			close(fd1[1]);
	  		execvp(parsedPipe[0], parsedPipe);
		}
		else if (p2 > 0){
			//setpgid(p2, p2);
			//tcsetpgrp(0, p2);
		}
		else if(p2 < 0){
			printf("\nFork Error");
			return;
		}
	}else{
		printf("\nFork Error");
		return;
	}
	//closing fds IMPORTANT
  	close(fd1[0]);
  	close(fd1[1]);

	//waiting
  	for (i = 0; i < 2; i++)
	{
		wait(&status);
	}
}

//Implemented by Suleiman
//exec commands piped 3
void execCmdPipes3(char **parsed, char **parsedPipe, char **parsedPipe2){

		int status;
		int i;
		int fd1[2], fd2[2];
		//creating 2 pipes so we pipe from cmd 1 to cmd 2 and then from cmd 2 to cmd3
		pipe(fd1);
		pipe(fd2);
		
		int p1=fork();
		if (p1 == 0)//child process
		{
			setpgid(0,0);	//gives unique pgid to the child
			//tcsetpgrp(0, getpgrp());    //sets child to be part of the foreground process

			//executing first command before |
			dup2(fd1[1],1);
	  		
			close(fd1[0]);
			close(fd1[1]);
			close(fd2[0]);
			close(fd2[1]);

			execvp(parsed[0], parsed);
 	 	}
  		else if(p1 > 0)// parent
        {
			//setpgid(p1, p1);
			//tcsetpgrp(0, p1);
			int p2 = fork();
            if (p2 == 0)	//child
			{
				setpgid(0,0);	//gives unique pgid to the child
				// tcsetpgrp(0, getpgrp());    //sets child to be part of the foreground process
				//executing second command after first |
	  			dup2(fd1[0], 0);
				dup2(fd2[1], 1);

	  			close(fd1[0]);
				close(fd1[1]);
				close(fd2[0]);
				close(fd2[1]);	  			

	  			execvp(parsedPipe[0], parsedPipe);
			}
			else if(p2 > 0)// parent
			{	
				//setpgid(p2, p2);
				//tcsetpgrp(0, p2);
				int p3 = fork();
	  			if (p3 == 0)	//child process
                {
					setpgid(0,0);	//gives unique pgid to the child
					//tcsetpgrp(0, getpgrp());    //sets child to be part of the foreground process
					//executing third command after second |
	      			dup2(fd2[0],0);

	      		    close(fd1[0]);
			        close(fd1[1]);
			        close(fd2[0]);
			        close(fd2[1]);      				
	      				
                    execvp(*parsedPipe2, parsedPipe2);
				}else if (p3 > 0) {
					//setpgid(p3,p3);	//gives unique pgid to the child
					//tcsetpgrp(0, p3);    //sets child to be part of the foreground process
				} else if(p3 < 0){
					printf("\nFork Error");
					return;
				}
			}else{
				printf("\nFork Error");
				return;
			}
		}else{
			printf("\nFork Error");
			return;
		}
	  
	  	//closing FDs IMPORTANT
  		close(fd1[0]);
  		close(fd1[1]);
  		close(fd2[0]);
  		close(fd2[1]);

		//waiting for status
  		for (i = 0; i < 3; i++)
		{
				wait(&status);
		}
}

int ownCmdHandler(char** parsed)
{/*
this function returns 1 if commands are supported
otherwise returns 0
*/
	//We will write our commands here
	//Implemented by Suleiman
	//cd

	//myhistory
	if(strcmp(parsed[0], "myhistory") == 0){	//if myhistory is typeded
		int e=0;
		if(parsed[1] == NULL);
		else if(strcmp(parsed[1], "-c") == 0)
		{
			qsize=0;
			front=0;
			rear=0;
			return 1;
		}
		else if(strcmp(parsed[1], "-e") == 0)
		{
			int Temp=T(parsed[2]);
			parsed[0][0]='\0';
			parsed[1]=NULL;
			parsed[2]=NULL;
			Temp=(front+Temp-1)%qcap;
			strcpy(parsed[0],q[Temp]);
			parseSpace(parsed[0], parsed);
			e=1;
		}
		if(!e)
		{
			printHist();
			return 1;
		}
		//print out all the elements in the histCmds queue
	}

	if(strcmp(parsed[0],"cd")==0){
		chdir(parsed[1]);
		return 1;
	}

	//exit implemented by Brandon Sharp
	if(strcmp(parsed[0], "exit") == 0) //if exit is typeded
	{
		exit(0);
		return 1;
	}

	//path
	if(strcmp(parsed[0], "path") == 0) //if path is typeded
	{
		if(pathnum == 0) pathinit();
		if(parsed[1] == NULL)
		{
			getpath();
		}
		else if(strcmp(parsed[1],"+") == 0){
			char p1[2550] = "PATH=",p2[2550] = ":";
			pathnum++;
			strcpy(path[pathnum],parsed[2]+1);
			printf("PATH added\n");
		}
		else{
			char p1[2550]="PATH=",p2[2550]=":";
			strcat(p1,path[1]);
			for(int i=2;i<=pathnum;i++)
			{
				if(strcmp(parsed[2]+1, path[i]) == 0)
				{
					path[i][0] = '\0';
				}
				else
				{
					strcat(p1,p2);
					strcat(p1,path[i]);
				}
			}
			putenv(p1);
			printf("PATH removed\n");
		}
		return 1;
	}

	return 0;
}


//Implemented by Suleiman
//creates array of strings from input separated by space
void parseSpace(char* str, char** parsed)
{
	int i;

	for (i = 0; i < MAXLIST; i++) {
		parsed[i] = strsep(&str, " ");
		if (parsed[i] == NULL)
			break;
		if (strlen(parsed[i]) == 0){
			i--;
		}
	}
}

//Implemented by Suleiman
//creates array of strings from input separated by semicolon
int parseSemi(char* str, char** parsed) {

	char *cmd;
	int i = 0;

	while((cmd = strsep(&str,";")) != NULL) {
		
		//skipping spaces in the beginning
		while(cmd[0] == ' ') {
			cmd++;
		}
	
		if(strlen(cmd) > 1) {
			if(cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}
			parsed[i] = cmd;
			i++;
		}
	}
	return i;

}

//Implemented by Suleiman
//creates array of strings from input separated by pipe
int parsePipe(char* str, char** parsed) {

	char *cmd;
	int i = 0;

	while((cmd = strsep(&str,"|")) != NULL) {
		
		//if starts with spaces
		while(cmd[0] == ' ') {
			cmd++;
		}
	
		if(strlen(cmd) > 1) {
			if(cmd[strlen(cmd) - 1] == '\n') {
				cmd[strlen(cmd) - 1] = '\0';
			}
			parsed[i] = cmd;
			i++;
		}
	}
	return i;

}

//Implemented by Brandon Sharp
//Executes redirection symbols < and > 
void processRedirection(char* str, char** parsed,int loc)
{
	char* filename = strtok(parsed[loc+1], " ");
	pid_t id = fork();
	if(id == 0)
	{
		//Doing redirection if either if statement is true
		if(strcmp(parsed[loc], "<") == 0) //if redirect read
		{
			//reading filename
			printf("Filename for input redirection %s\n", filename);
			int fd2 = open(filename, O_RDONLY);
			if(fd2 < 0) //if bad pipe
			{
				printf("Couldn't run\n");
				exit(0);
			}
			//creating a pipe
			parsed[loc]=NULL;
			parsed[loc+1]=NULL;
			dup2(fd2, 0);
			close(fd2);;
			execvp(parsed[0], parsed); //executes the cmd arguments
			printf("%s: command not found\n", parsed[0]); //should never execute
			exit(0);
		}
		else if(strcmp(parsed[loc], ">") == 0) //if redirect writes
		{
			//reading filename
			printf("Filename for output redirection %s\n", filename);
			int fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
			if(fd < 0)
			{
				printf("Couldn't run\n");
				exit(0);
			}
			parsed[loc]=NULL;
			parsed[loc+1]=NULL;
			// creating a pipe
			dup2(fd, 1);
			close(fd);
			execvp(parsed[0], parsed); //executes the cmd arguments
			printf("%s: command not found\n", parsed[0]); //should never execute
			exit(0);
		}
		free(filename); //frees the pointer
	}
	else if(id > 0) //parent wait for the child
	{
		wait(NULL);
	}
}

//implemented by mingzhi
char* ExecuteAlias(char* str)
{
	//alias
	char tmp2[255];
	strncpy(tmp2,str,5);
	tmp2[5]='\0';
	// printf("%s\n",tmp2);
	if(strcmp(tmp2, "alias") == 0) //if alias is typeded
	{
		if(strlen(str)==5)
		{
			str[0]='\0';
			printf("Alias List:\n");
			for(int i=0;i<Aliasnum;i++)
			{
				if(Alias1[i][0]!='\0')
				{
					printf("%s = %s\n",Alias1[i],Alias2[i]);
				}
			}
			return str;
		}
		if(str[6]=='-'&&str[7]=='r')
		{
			char rm[255];
			for(int i=9;i<strlen(str);i++)
			{
				rm[i-9]=str[i];
			}
			rm[strlen(str)-9]='\0';
			// printf("rm=%s\n",rm);
			for(int i=0;i<Aliasnum;i++)
			{
				if(strcmp(rm, Alias1[i]) == 0)
				{
					Alias1[i][0]='\0';
				}
			}
			str[0]='\0';
			return str;
		}
		if(str[6]=='-'&&str[7]=='c')
		{
			Aliasnum=0;
			str[0]='\0';
			return str;
		}
		int i=6,j=0;
		for(;i<strlen(str);i++)
		{
			if(str[i]==' ')
			{
				printf("alias: usage: alias name='value'");
				str[0]='\0';
				return str;
			}
			if(str[i]=='=')
			{
				Alias1[Aliasnum][j]='\0';
				if(str[i+1]!='\'')
				{
					printf("alias: usage: alias name='value'");
					str[0]='\0';
					return str;
				}
				j=0;i+=2;
				break;
			}
			Alias1[Aliasnum][j++]=str[i];
		}
		for(;i<strlen(str);i++)
		{
			if(str[i]=='\'')
			{
				Alias2[Aliasnum++][j]='\0';
				break;
			}
			Alias2[Aliasnum][j++]=str[i];
		}
		if(str[i]!='\'')
		{
			printf("alias: usage: alias name='value'");
			Aliasnum--;
		}
		str[0]='\0';
		// printf("Alias : %s %s\n",Alias1[Aliasnum-1],Alias2[Aliasnum-1]);
		return str;
	}

	//check alias
	for(int i=0;i<Aliasnum;i++)
		if(strcmp(str, Alias1[i]) == 0) //if an alias is typeded
		{
			str[0]='\0';
			strcat(str,Alias2[i]);
			str[strlen(Alias2[i])]='\0';
			break;
		}
	return str;
}

//scans the input string parses pipes, semicolons and spaces, then runs commands
void analString(char* str, char** parsed, char **parsedPipe, char **parsedPipe2)
{
	str=ExecuteAlias(str);
	char **parsedCMD[64];
	char *r, *w;
	//parsing semicolons
	int s = parseSemi(str, parsedCMD);

	//for every semiclon separated command and his args do..
	for (int i = 0; i < s; i++){
		char *stripped[3];
		//parsing pipes
		int pipe = parsePipe(parsedCMD[i], stripped);

		if(pipe==2){//if 2 commands and 1 pipe
			parseSpace(stripped[0],parsed);
			parseSpace(stripped[1],parsedPipe);
			execCmdPipes2(parsed, parsedPipe);
		}else if(pipe==3){//if 3 commands and 2 pipes
			parseSpace(stripped[0],parsed);
			parseSpace(stripped[1],parsedPipe);
			parseSpace(stripped[2], parsedPipe2);
			execCmdPipes3(parsed, parsedPipe, parsedPipe2);
		}else if(pipe == 1){//if 1 command and 0 pipe
			//processes redirection
					if(((r = strstr(str, "<")) != NULL) || ((w = strstr(str, ">")) != NULL)) //if the input line contains < or >
					{
						parseSpace(stripped[0],parsed);
						int j=0;
						for(j=0;j<5;j++)
							if(strcmp(parsed[j],"<")==0||strcmp(parsed[j],">")==0)
							{
								processRedirection(parsedCMD[i], parsed,j);
								break;
							}
						if(j!=5)break;
					}
			else
			{
			parseSpace(parsedCMD[i], parsed);
			}
			int stat = ownCmdHandler(parsed);
			//if no pipes, execute built in commands
			if(stat==0){
				execCommands(parsed);
			}
		}		
	 }

}

int main(int argc, char **argv)//main will take in command arguements
{
	char inputString[MAXCOM]; //array to hold entire command input

	char *parsedArgs[MAXLIST]; // array of strings
	char *parsedArgsPiped[MAXLIST]; // array of strings
	char *parsedArgsPiped2[MAXLIST]; // array of strings
	
	pid_t shell_pgid;
	int shell_fd = STDIN_FILENO;

	while(tcgetpgrp (shell_fd) != (shell_pgid = getpgrp())){
		kill( - shell_pgid, SIGTTIN);
	}

	shell_pgid = getpid();

	if(setpgid(shell_pgid, shell_pgid) < 0){
		perror("Couldn't put the shell in its own process group");
      	exit(1);
    }
	tcsetpgrp(shell_fd, shell_pgid);	//sets the foreground

	signal(SIGINT, sig_handler_p);		//<---responsible for ignoring sigint
	signal(SIGTSTP, sig_handler_p);		//<---responsible for ignoring sigint

	
	//signal(SIGTSTP, SIG_IGN);		//<---responsible for ignoring sigint

	int q[21];
	
	//pathinit(); // if this thing is uncommented not built in functions crash
	printf("Welcome to Aboba shell\n");
	printf("Property of SMJB\n");

	if(argc==1){//interactive mode
		while (1) {
			pwdFunc();
			if (lineInput(inputString)==1){
			continue;
			}
			Enqueue(inputString);
			//printf("%s\n",queue->histCmds[0]);
			analString(inputString,parsedArgs, parsedArgsPiped, parsedArgsPiped2);
		}
	}else if(argc > 1){//batch mode
		FILE *fp;
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			perror("Failed: Batch file does not exist or can't be opened");
			return 1;
		}
		while (fgets(inputString, MAXCOM - 1, fp)){
		inputString[strcspn(inputString, "\n")] = 0;
		printf("Executing: \"%s\" in batch mode...\n", inputString);
		analString(inputString,parsedArgs, parsedArgsPiped, parsedArgsPiped2);
		}

		fclose(fp);

	}
	setenv("PATH",path[0],1);
	return 0;
	
}
//checkf
