#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "/usr/include/mysql/mysql.h"

#define BUFSIZE 1000

#define DB_HOST "127.0.0.1"
#define DB_USER "moon"
#define DB_NAME "moon_db"
#define DB_PASSWORD "1234"

void* clnt_connection(void * arg);
void send_message(char* message, int len);
void error_handling(char * message);
char *sub_str(char *word, int len);

int clnt_number=0;
int clnt_socks[20];
	
char *str;

MYSQL *conn;
MYSQL_RES* res;
MYSQL_ROW row;
int fields;

pthread_mutex_t mutx;

int main(int argc, char **argv)
{
	// socket create

	int serv_sock;
	int clnt_sock;
	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	int clnt_addr_size;
	pthread_t thread;

	if(argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	
	if(pthread_mutex_init(&mutx,NULL))
		error_handling("mutex init error");
		
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);
	if(serv_sock == -1)
		error_handling("socket() error");
	//Prepare
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));
	
	//bind()
	if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("bind() error");
	
	//listen()
	if(listen(serv_sock, 5) == -1)
		error_handling("listen() error");
	printf("Wating Connect....\n");	
	//Database Connect
	conn = mysql_init(NULL);
	if(!mysql_real_connect(conn,DB_HOST,DB_USER,DB_PASSWORD,DB_NAME,0,NULL,0))
	{
		printf("Connect Error\n");
		exit(1);
	}
	printf("Database Connect\n");

	while(1)
	{
		clnt_addr_size = sizeof(clnt_addr);	
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
       if(0> clnt_sock)
	   {
		   printf("accept() failed\n");
		   continue;
	   }
		pthread_mutex_lock(&mutx);
       
		clnt_socks[clnt_number++]=clnt_sock;
		pthread_mutex_unlock(&mutx);
		
		pthread_create(&thread, NULL, clnt_connection, (void*)&clnt_sock);
		pthread_detach(thread);
		printf(" IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
		
	}

	mysql_free_result( res );
	mysql_close(conn);
	return 0;
	
}

void *clnt_connection(void *arg)
{
	int clnt_sock =*( (int*) arg);
	int str_len=0;
	char message[BUFSIZE];
	int i = 0;
	int field;
//	memset(message,0,sizeof(message));
	char *sendmsg;
	char *msg;
	char db_op[BUFSIZE];
	while((str_len=read(clnt_sock,message,sizeof(message))) != 0)
	{
		if(strncmp(message,"/list",5) == 0)
		{
			mysql_query(conn,"SELECT * from messages");

			res = mysql_store_result(conn);
			field = mysql_num_fields(res);
		
			while(row = mysql_fetch_row(res))
			{
				for(i = 0; i<field; i++)
				{
					printf("%s ",row[i] ? row[i] : "NULL");
					strcat(row[i],"\n");
					sendmsg = row[i];
					send_message(sendmsg, strlen(sendmsg));
				}
				printf("\n");
			}

		}
		else if(strncmp(message,"/c",2) == 0)
		{
			str = sub_str(message,3);
			sprintf(db_op,"insert into messages values (\'%s\')",str);
			mysql_query(conn,db_op);
//			strcat(
//			printf("%s",str);
//			memset(str,0,sizeof(str));
			free(str);

			sendmsg = "Create Sucess\n";
			send_message(sendmsg, strlen(sendmsg));
		}
		else if(strncmp(message,"/d",2) == 0)
		{
			sub_str(message,3);
			sprintf(db_op,"delete from messages where msg=\'%s\'",str);
			mysql_query(conn,db_op);
			sendmsg = "Delete Success\n";
			send_message(sendmsg, strlen(sendmsg));
		}
		else if(strncmp(message,"/p",2) == 0)
		{
			sub_str(message,3);
			sprintf(db_op,"select * from messages where msg=\'%s\'",str);
			if( mysql_query(conn,db_op) != 0)
			{
				sendmsg = "Print failed..!\n";
				send_message(message, strlen(message));
			}
			else
			{
				send_message(str, strlen(str));
			}
		}
		else if(strncmp(message,"Connecting check..Ack Request",29) == 0)
		{
			sendmsg = "Ack Message Send..!\n";
			send_message(sendmsg, strlen(sendmsg));
		}
		else
		{
			sendmsg = "The Operation is incorrect\n";
			send_message(sendmsg, strlen(sendmsg));
		}
	
	}
	pthread_mutex_lock(&mutx);
	for(i=0;i<clnt_number;i++){ 	//remove disconnected client
		if(clnt_sock == clnt_socks[i])
		{
			while(i++ < clnt_number-1)
				clnt_socks[i] = clnt_socks[i+1];
			memset(sendmsg,0,sizeof(sendmsg));
			break;
		}
	}
	clnt_number--;
  
	pthread_mutex_unlock(&mutx);

	close(clnt_sock);
	return 0;
}

void send_message(char * message, int len)	//send to all
{
	int i;
	pthread_mutex_lock(&mutx);
  
	for(i=0;i<clnt_number;i++)
	{
		send(clnt_socks[i],message,len,0);
	}
		
	pthread_mutex_unlock(&mutx);

	
}

char *sub_str(char *word, int len)
{
	int i;
	int size_word = strlen(word) - len;
	str = malloc(size_word);

	for(i=0; i<strlen(word)-len; i++)
	{
			str[i] = word[i+len];
	}	

	str[size_word-1] = '\0';

	return str;
}

void error_handling(char * message)
{
	fputs(message, stderr);
	fputc('\n',stderr);
	exit(1);
}
