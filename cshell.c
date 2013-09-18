/*
AUTHOR : SOMYA MEHDIRATTA
ROLL NO : 201201007 
----------------------------------------------------------------------------------------------------------------------------------------
 ---------------------------------------------------- INTERACTIVE BASH LIKE SHELL -----------------------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------
*/

#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/wait.h>
#include <signal.h>
#include<sys/utsname.h>

// ----- for background processes ------
typedef struct back
{
	char name[100];
	int pi;
	int state;
}back;
back background[100];  //store max 100 processes
int back_c=0,curid=0; 
// ------- for prompt details ---------
char hostname[101],currdir[101],homedir[100];
int length_home=0;

// --------------------------------------------------- PARSING  ---------------------------------------------------------------------
int split(char input[],char db[][100]) {
	char copy[100];
	strcpy(copy,input);
	int size=0;
	char *temp;
	temp=strtok(input," \t	");
	while (temp != NULL) {
		strcpy(db[size],temp);
		size++;
		temp=strtok(NULL," \t	");
	}
	strcpy(input,copy);
	return size;
}

// ------------------------------------------------- CHANGE DIRECTORY---------------------------------------------------------------------
void cd(char *input,char db[][100],int size){
	if((size == 1) || (!strcmp(db[1],"~")) || (!strcmp(db[1],"~/"))){
		chdir(homedir);
		return;
	}
	else {
		int ret=chdir(db[1]);
		if(ret==-1)
			printf("%s: No such file or directory\n",db[1]);
	}
}
// --------------------------------------------------- USER PROMPT ---------------------------------------------------------------------
void prompt(){
	int x=gethostname(hostname,101);
	char *dir=getcwd(currdir,101);
	if(strlen(currdir) < length_home)
		printf("<%s@%s:%s> ",getlogin(),hostname,dir);
	else {
		dir+=length_home;
		printf("<%s@%s:~%s> ",getlogin(),hostname,dir);
	}
}
// ------------------------------------------------- EXECUTING EXISTING COMMANDS -----------------------------------------------------
void execute(char* input,char db[][100],int size){
	char *st[100]; int i;
	for (i = 0; i < size; i++) 
		st[i]=db[i];
	st[size]=NULL;
 	int mode=0; // 0 : fore |-------|   1  : back 
	if(strstr(input,"&"))  { //Background Process
		mode=1;
		st[size-1]=NULL;
	}
	pid_t pid=fork();
	if(pid<0){
		printf("** ERROR : Forking child process failed **\n");
		exit(1);
	}
	if(pid==0){
		int err=execvp(st[0],st);
		if(err<0){
			printf("Command not found\n");
			_exit(0);
		}
	}
	else {if(mode==0){
		int somya;
		curid=pid;
		if(waitpid(pid,&somya,WUNTRACED)<0)
			perror("Wait pid\n");
		if(WIFSTOPPED(somya)){
			background[back_c].pi=pid;
		        background[back_c].state=1;
			strcpy(background[back_c].name,db[0]);
			back_c++;
		}
		//wait(NULL);
		}
		else {
		 	//printf("Back Process running\n");  // Background Process
			background[back_c].pi=pid;
			background[back_c].state=1;
			char temp[200];
			strcpy(temp,st[0]);
			int x=1;
			while(x<size-1){
			strcat(temp," ");
			strcat(temp,st[x++]);
			}
			strcpy(background[back_c].name,temp);
			back_c++;

		}
	}
}
// ----------------------------------------------------- SIGNAL HANDLING -----------------------------------------------------------------
void sig_handle(int sign)
{
	if (sign==2 ||sign==3)
	{
		fflush(stdout);
		printf("\n");
		prompt();
		signal(SIGQUIT,sig_handle);
		signal(SIGINT,sig_handle);
		
	}
		if(sign == 20)
			kill(curid,SIGTSTP);
	
	return;
}
void child_sig(int signo)
{
	pid_t pid;
	int r;
	pid=waitpid(WAIT_ANY,&r, WNOHANG);
	int i;
	for(i=0;i<back_c;i++)
	{
		if(background[i].pi==pid && background[i].state==1)
		{
			background[i].state=0;
			printf("\n%s %d exited normally\n",background[i].name,background[i].pi);
			prompt();
			fflush(stdout);
		}
	}
	signal(SIGCHLD, child_sig);
	return;
}
// -------------------------------------------------- USER DEFINED FUNCTIONS -----------------------------------------------------------
void pinfo(char db[][100],int size){
	pid_t id;
	char msg[1000];
	if(size==1)
		id=getpid();
	else
		id=background[atoi(db[1])-1].pi;
	printf("pid -- %d\n",id);
	char path[100],jid[200];
	strcpy(path,"/proc/");
	sprintf(jid,"%d",id);
	strcat(path,jid);
	strcat(path,"/stat");
	FILE * f1;
	char stat,temp[200];
	f1=fopen(path,"r");
	if(f1==NULL)
	{
		perror("error\n");
	}
	printf("pid -- %d\n",id);
	fscanf(f1,"%s",temp);
	fscanf(f1,"%c",&stat);
	fscanf(f1,"%s",temp);
	fscanf(f1,"%c",&stat);
	fscanf(f1,"%c",&stat);
	printf("Process status - %c\n",stat);
	long long unsigned i,vsize;
	for(i=0;i<20;i++)
	{
		fscanf(f1,"%c",&stat);
		fscanf(f1,"%llu",&vsize);
	}
	printf("Memory - %llu\n",vsize);
	fclose(f1);
	strcpy(path,"/proc/");
	strcat(path,jid);
	strcat(path,"/exe");
	int len=readlink(path,temp,200);
	temp[len]='\0';
	printf("Executable Path - %s\n",temp);

}
void jobs(){
	int i,j=1;
	for (i = 0; i <back_c ; i++) 
		if(background[i].state==1)
			printf("[%d] %s [%d]\n",j++,background[i].name,background[i].pi);
}
void kjob(char db[][100]){
	int pno=atoi(db[1]);
	int i,j=0;
	for(i = 0; i < back_c ; i++)
	{
		if(background[i].state==1)
			j++;
		if(j==pno)
			break;
	}
	if(i==back_c)
		printf("No such job exists\n");
	else
		kill(background[i].pi,atoi(db[2]));

}
void fg(char db[][100]){
	int pno=atoi(db[1]),s,i,j;
	for(i = 0; i < back_c ; i++)
	{
		if(background[i].state==1)
			j++;
		if(j==pno)
			break;
	}
	if(i==back_c)
		printf("No such background process exists\n");
	else
	{
		waitpid(background[i].pi,&s,0);
		background[i].pi==0;
	}
}
void overkill(){
	int i;
	for (i = 0; i < back_c; i++) {
		if(background[i].state==1)
			kill(background[i].pi,9);
	}
	back_c=0;
}
// ---------------------------------------------------- MAIN FUNCTION ------------------------------------------------------------------
int main(int argc, const char *argv[])
{
	//  -------- Signal Handling ------------
	signal(SIGINT,SIG_IGN);
	signal(SIGINT,sig_handle);
	signal(SIGCHLD,SIG_IGN);
	signal(SIGCHLD,child_sig);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTSTP,sig_handle);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGQUIT,sig_handle);
	//  -------- Prompt extended ------------
	char *r=getcwd(currdir,101);
	int i=0;
	strcpy(homedir,currdir);
	while(currdir[i++]!='\0'){
		++length_home;
	}
	char input[1001]={'\0'};
	// ---------------- main ----------------
	while(1){
		prompt();
		scanf("%[^\n]",input);
		if(strstr(input,"exit"))
			break;
		char db[100][100];
		int size=split(input,db);
		getchar();
		if(strcmp(db[0],"")!=0){
			if(!strcmp(db[0],"cd"))
				cd(input,db,size);
			else if(!strcmp(db[0],"quit"))
				exit(0);
			else if(!strcmp(db[0],"pinfo"))	
				pinfo(db,size);
			else if(!strcmp(db[0],"jobs"))
				jobs();
			else if(!strcmp(db[0],"kjob"))
				kjob(db);
			else if(!strcmp(db[0],"fg"))
				fg(db);
			else if(!strcmp(db[0],"overkill")) 
				overkill();
			else 
				execute(input,db,size);
		}
	}
	return 0;
}
