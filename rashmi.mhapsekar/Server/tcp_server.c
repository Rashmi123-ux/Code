#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>


#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;

/***
	@brief  : Flush the stdout
	@param  : void
	@return : void 
*/
void str_overwrite_stdout()
{
	printf("\r%s", "> ");
	fflush(stdout);
}

/***
	@brief  : Trim line feed 
	@param  : pointr to char array, characters length
	@return : void 
*/
void str_trim_lf(char *arr, int length)
{
	for(int i = 0; i<length; i++)
	{
		if(arr[i] == '\n')
		arr[i] = '\0';
		break;
	}

}
//Client Structure
typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
}client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER; 


/***
	@brief  : Add clients in a queue
	@param  : pointr to client structure
	@return : void 
*/
void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);
	for(int i = 0; i<MAX_CLIENTS; i++)
	{
		if(!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

/***
	@brief  : Remove client from queue
	@param  : user id
	@return : void 
*/
void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);
	for(int i = 0; i<MAX_CLIENTS; i++)
	{
		if(clients[i])
		{
	 		if(clients[i]->uid == uid)
	 		{
	 			clients[i] = NULL;
	 			break;
	 		}
	 		
		}
	}
	pthread_mutex_unlock(&clients_mutex);
}

/***
	@brief  : Print client IP address
	@param  : Socket address struct
	@return : void 
*/
void print_ip_addr(struct sockaddr_in addr)
{
	printf("%d.%d.%d.%d",
		addr.sin_addr.s_addr & 0xff,
		(addr.sin_addr.s_addr & 0xff00) >> 8,
		(addr.sin_addr.s_addr & 0xff0000) >> 16,
		(addr.sin_addr.s_addr & 0xff000000) >> 24
		);
	
}

/***
	@brief  : Send message to connected client 
	@param  : Data, user id
	@return : void 
*/
void send_message(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);
	for(int i = 0; i<MAX_CLIENTS; i++)
	{
		
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				if(write(clients[i]->sockfd, s, strlen(s)) < 0 )
				{
					printf("ERROR: write to descriptor failed\n");
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&clients_mutex);	
}

/***
	@brief  : handle client
	@param  : Pointer to client struct
	@return : void pointer
*/
void *handle_client(void *arg)
{
	char buffer[BUFFER_SZ];
	char message[BUFFER_SZ];
	int leave_flag = 0, n = 0;
	cli_count++;
	char ch;
	
	client_t *cli = (client_t*)arg;
	
	FILE *p;
	
	bzero(buffer, BUFFER_SZ);
	bzero(message, BUFFER_SZ);
	
	while(1)
	{
		if(leave_flag)
		{
			break;
		}
		int receive = recv(cli->sockfd, buffer, BUFFER_SZ, 0);

		if(receive > 0)
		{
			if(strlen(buffer) > 0)
			{
				p = popen(buffer, "r");
				if(p == NULL)
				{
					printf("Error: Enable to open popen\n\r");
						
				}	
				while( (ch = fgetc(p) ) != EOF)
				{
					message[n++] = ch; 
				}
				send_message(message, cli->uid);
				printf("%s", message);
				bzero(message, BUFFER_SZ);
				pclose(p);
			}
		}else if(receive == 0 || strcmp(buffer, "bye") == 0){
			sprintf(buffer, "%d has left\n", cli->uid);
			printf("%s\n", buffer);
			send_message(buffer, cli->uid);
			leave_flag = 1;
		}else
		{
			printf("ERROR: -1\n");
			leave_flag = 1;
		}
		bzero(buffer, BUFFER_SZ);
		n = 0;
		
	}
	
	bzero(buffer, BUFFER_SZ);
	bzero(message, BUFFER_SZ);
	
	close(cli->sockfd);
	queue_remove(cli->uid);
	free(cli);
	cli_count--;
	pthread_detach(pthread_self());
	
	return NULL;
	
}

//Main function
int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;
	
	//Socket settings
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if(listenfd == -1)
	{
		printf("ERROR: Socket creation failed...\n");
		return EXIT_FAILURE;
	}
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family =  AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
	serv_addr.sin_port = htons(port);
	
	//Signals
	signal(SIGPIPE, SIG_IGN);
	
	if(setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0)
	{
		printf("ERROR: setsocket\n");
		return EXIT_FAILURE;
	}
	
	//bind
	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("ERROR: bind\n");
		return EXIT_FAILURE;	
	}
	
	//listen
	//baclog size set to 10
	if(listen(listenfd, 10) < 0)
	{
		printf("ERROR: listen\n");
		return EXIT_FAILURE;		
	}
	
	printf("---Welcome to the Socket Communication with multiple clients---\n");
	
	while(1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
		
		//check for max client
		if((cli_count + 1) == MAX_CLIENTS)
		{
			printf("Maximum clients connected. Connection Rejected\n");
			print_ip_addr(cli_addr);
			close(connfd);
			continue;
		}
	//Client setting
	client_t *cli = (client_t *)malloc(sizeof(client_t));
	cli->address = cli_addr;
	cli->sockfd = connfd;
	cli->uid = uid++;
	
	queue_add(cli);	
	pthread_create(&tid, NULL, &handle_client, (void*)cli);
		
		
	//Reduce CPU usage
	sleep(1);
	}
	
	return EXIT_SUCCESS;
}



