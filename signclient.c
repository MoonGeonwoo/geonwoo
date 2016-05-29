#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFSIZE 1000
#define NAMESIZE 20

void* send_message(void* arg);
void* recv_message(void* arg);
void error_handling(char * message);
int kbhit();

char name[NAMESIZE] = "[Default]";
char checkmsg[BUFSIZE] = "Connecting check..Ack Request\n";
char message[BUFSIZE];
int main(int argc, char **argv)
{
//	char checkmsg[BUFSIZE] = "Connecting check..Ack Request\n";	
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void* thread_result;

	if(argc != 4)
	{
		printf("Usage : %s <ip> <port> \n", argv[0]);
		exit(1);
	}

	sprintf(name, "[%s]", argv[3]);	
	sock = socket(PF_INET, SOCK_STREAM, 0);

	if(sock == -1)
		error_handling("socket() error");
	//prepare
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr(argv[1]);
	serv_addr.sin_port=htons(atoi(argv[2]));

	if(connect(sock,(struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error!");

	pthread_create(&snd_thread, NULL, send_message, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_message, (void*)&sock);

	pthread_join(snd_thread, &thread_result);
	pthread_join(rcv_thread, &thread_result);
	
	close(sock);
	return 0;
}

void* send_message(void * arg)
{
	int sock =*((int*) arg);
	int timer = 0;
	memset(message,0,strlen(message));
//	char name_message[NAMESIZE+BUFSIZE];
	while(1){
		timer++;
		sleep(1);
		if(timer == 3)
		{
			printf("Connecting check..Ack Request\n");
			send(sock,checkmsg,strlen(checkmsg),0);
			timer = 0;
		}
		else if(timer <3 && kbhit() == 1)
		{
			fgets(message,BUFSIZE,stdin);
			send(sock,message,strlen(message),0);
			timer = 0;
		}
		if(!strcmp(message, "/q\n")) {
	  		close(sock);
			exit(0);
	  	}
//	  sprintf(name_message, "%s %s", name, message);
//	  write(sock, name_message, strlen(name_message));
	}
}

void* recv_message(void* arg)
{
	int sock = *((int*) arg);
	char name_message[NAMESIZE+BUFSIZE];
	memset(name_message,0,strlen(name_message));
	char buffer[BUFSIZE];
	memset(buffer,0,strlen(buffer));
	int str_len;
	while(1)
	{
		str_len = recv(sock, buffer, BUFSIZE,0);
		if(str_len == -1) 
			return (void*)-1;
		name_message[str_len]=0;
//		fputs(name_message,stdout);
		printf("from server, Message arrived. \n%s",buffer);
		memset(buffer,0,sizeof(buffer));
		printf("\n");

	}
}

void error_handling(char * message)
{
	fputs(message ,stderr);
	fputc('\n', stderr);
}

int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO,&oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW,&newt);
	oldf = fcntl(STDIN_FILENO,F_GETFL,0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW,&oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch,stdin);
		return 1;
	}

	return 0;
}
