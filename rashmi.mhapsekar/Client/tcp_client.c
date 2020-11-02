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
#include <pthread.h>
#include <signal.h>


#define BUFFER_SZ 2048

volatile sig_atomic_t flag = 0;
int socketfd = 0;


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
/***
	@brief  : catch_ctrl_c_and_exit
	@param  : void
	@return : void 
*/
void catch_ctrl_c_and_exit()
{
	flag = 1;
	
}

/***
	@brief  : Receive message from server
	@param  : void
	@return : void 
*/
void recv_msg_handler()
{
	char message[BUFFER_SZ] = {};
	while(1)
	{
		int receive = recv(socketfd, message, BUFFER_SZ, 0);
		if(receive > 0)
		{
			printf("%s\n", message);
			str_overwrite_stdout();
		}
		else if(receive == 0)
		{
			break;
		}
		bzero(message, BUFFER_SZ);
	}
}

/***
	@brief  : Send message to server
	@param  : void
	@return : void 
*/
void send_msg_handler()
{

	char message[BUFFER_SZ] = {};
	while(1)
	{
		fgets(message, BUFFER_SZ, stdin);
		if(strncmp(message, "bye",3) == 0)
		{
			flag = 1;
			break;
		}
		else
		{
			send(socketfd, message, strlen(message), 0);
		}				
		bzero(message, BUFFER_SZ);
	}
}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);	
	
	signal(SIGINT, catch_ctrl_c_and_exit);
	

	struct sockaddr_in servAddr;
	
	socketfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(socketfd < 0)
	{
		printf("Error: Socket Failure\n");
		return EXIT_FAILURE;
	}
	
	bzero(&servAddr,sizeof(servAddr));
	
	servAddr.sin_family =  AF_INET;
        servAddr.sin_addr.s_addr = inet_addr(ip);
	servAddr.sin_port = htons(port);
	
	//connect to server with given IP
	if(connect(socketfd, (struct sockaddr*)&servAddr, sizeof(servAddr)) != 0)
	{
		printf("Error : connect with server failed\n");
	}
	
	printf("---Welcome to the Socket Communication with multiple clients---\n");
	
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void*)send_msg_handler, NULL) != 0)
	{
		printf("Error: pthread\n");
		return EXIT_FAILURE;
	}
	
	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL, (void*)recv_msg_handler, NULL) != 0)
	{
		printf("Error: pthread\n");
		return EXIT_FAILURE;
	}
	while(1)
	{
		if(flag)
		{
			printf("\nbye\n");
			break;
		}
	}
	
	close(socketfd);
	
	return EXIT_SUCCESS;	
	
}
