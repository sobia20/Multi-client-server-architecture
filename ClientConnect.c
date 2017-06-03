#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <pthread.h>

#define BUFFERSIZE 64

void * writeToServer();

int sock;


int main()
{
	//int sock;
	struct sockaddr_in server;
	struct hostent *hp;
	char buf[1024];
	char connectInp[BUFFERSIZE];
	char * token;
	int portno;
	char serv_ip[BUFFERSIZE];
	
	while(1)
	{
		write(STDIN_FILENO, "Connect to server using IP and Port number\n",sizeof("Connect to server using IP and Port number\n")-1);
		read(STDIN_FILENO, connectInp, sizeof(connectInp)-1);
		token = strtok(connectInp," \n");
		
		if(strcasecmp(token,"connect")==0)
		{
			token = strtok(NULL, " ");
			strcpy(serv_ip, token);
			token = strtok(NULL, " ");
			portno = atoi(token);
			char b[BUFFERSIZE];
			
			sprintf(b,"IP is %s\n",serv_ip);
			write(STDOUT_FILENO,b, strlen(b));
			
				pthread_t writeToServer_thread;
				/* Create socket */
				sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0) 
				{
					perror("opening stream socket");
					exit(0);
				}
				/* Connect socket using name specified by command line. */
				server.sin_family = AF_INET;
				//hp = gethostbyname(argv[1]);
				hp = gethostbyname(serv_ip);
				if (hp == 0) 
				{
					char buffer[BUFFERSIZE];
					sprintf(buffer, "%s: unknown host\n", serv_ip);
					write(STDOUT_FILENO, buffer, strlen(buffer));
					exit(0);
				}
				bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
				//server.sin_port = htons(atoi(argv[2]));
				server.sin_port = htons(portno);
	
				if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) 
				{
					perror("connecting stream socket");
					exit(1);
				}
	
	
				int iret = pthread_create(&writeToServer_thread, NULL, writeToServer, NULL);
				if(iret)
				{
					char err[BUFFERSIZE];
					sprintf(err, "Error - pthread_create() return code: %d\n", iret);
					write(STDOUT_FILENO, err, strlen(err));
					exit(0);
				}
	
				int re;
				char output[BUFFERSIZE];
				while(1)
				{
					re = read(sock, output, sizeof(output)-1);
					if(re > 0)
					{
						write(STDOUT_FILENO, output, re);
					}
					if(re == 0)
					{
						close(sock);
					}
		
				  	if(strcasecmp(output, "exit")==0)
				  	{
				  		pthread_cancel(writeToServer_thread);
				  		close(sock);
				  		
				  		break;
				  	}
				  	if(strcasecmp(output, "disconnect")==0)
				  	{
				  		write(STDOUT_FILENO, "Disconnecting client\n",sizeof("Disconnecting client\n")-1);
				  		
				  		pthread_cancel(writeToServer_thread);
				  		close(sock);
				  		//pthread_exit(0);
				  		break;
				  		
				  	}
				}
			
			/*else
			{
				write(STDOUT_FILENO, "Enter ip again\,", sizeof("Enter ip again\n")-1);
			}*/
			exit(0);		
	
		}
		else
		{
			write(STDOUT_FILENO, "Invalid command\n",sizeof("Invalid command\n")-1);
		}
	}
	
}




void * writeToServer()
{
	while(1)
	{
		pthread_detach(pthread_self());
		char input[BUFFERSIZE]= " ";
		write(STDOUT_FILENO,"Please enter command\n",sizeof("Please enter command\n")-1);
		int re = read(STDIN_FILENO, input, sizeof(input)-1);
		input[re] = '\0';
		if(re == -1)
		{
			perror("client read error");
		}
		if (write(sock, input, strlen(input)) < 0)
			perror("writing on stream socket");
	}
	pthread_exit(0);
}

