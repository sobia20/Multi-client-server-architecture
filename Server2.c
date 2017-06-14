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
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define BUFFERSIZE 64


int i = 0;
int c = 0;
int executed = 0;


struct process
{
	clock_t start, end, elapsedTime;
	pid_t pid;
	char processName [50];
	char status[12];
	
}ptr[BUFFERSIZE];

struct client
{
	char ip[BUFFERSIZE];
	int portno;
	int clientfd;
	pid_t clientpid;
	int pipefd1[2];
	int pipefd2[2];
	
	
}clientInfo[BUFFERSIZE];

struct threadData
{
	int fd;
	char clientData[BUFFERSIZE];
};	

void loop(int);
long add(char *);
long sub(char *);
long mul(char *);
long divide(char *, int);
void help(int);
void signalHandler(int);
void * runProcess(void *);
void * list(void *);
void * killProcess(void *);
int createSocket(int);
int clientCode();
void * readThread(void *);
void * writeThread(void *);



void signal_handler(int signo)
{
	if(signo==SIGUSR1)
	{
		executed =1 ;
	}
}

void signal_handlerSIGCHLD(int signo)
{
	if(signo == SIGCHLD)
	{
		//update list
		int status;
		pid_t pid = waitpid(-1 , &status, WNOHANG);
		int j; 
		for(j = 0; j < i; j++)
		{
			if(ptr[j].pid == pid)
			{
				strcpy(ptr[j].status, "Terminated");
				ptr[j].end = clock();
				ptr[j].elapsedTime = ptr[j].end - ptr[j].start;
			}
		}
	}
}

int main(int argc, char *argv[])
{

	if (signal(SIGUSR1, signal_handler)== SIG_ERR)
	{
		perror("SIGUSR1");
	}
	if(signal(SIGCHLD, signal_handlerSIGCHLD) == SIG_ERR)
	{
		perror("Error in SIGCHLD");
	}
	int flag = 0;
	int sock = createSocket(flag);
	loop(sock);
	
}

int createSocket(int flag)
{
	int length;
	struct sockaddr_in server;
	
	char buf[1024];
	int rval;
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) 
	{
		perror("opening stream socket");
		exit(0);
	}
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	if(flag == 0)
	{
		write(STDOUT_FILENO,"Hi", 2);
		server.sin_port = htons(2626);
	}
	if(flag == 1)
	{
		server.sin_port = htons(2126);
	}
	if (bind(sock, (struct sockaddr *) &server, sizeof(server))) 
	{
		perror("binding stream socket");
		exit(0);
	}
	
	length = sizeof(server);
	if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*) &length)) 
	{
		perror("getting socket name");
		exit(0);
	}
	char sockbuff[BUFFERSIZE];
	sprintf(sockbuff, "Socket has port #%d\n", ntohs(server.sin_port));
	write( STDOUT_FILENO, sockbuff, strlen(sockbuff));
	
	return sock;
}

int clientCode()
{
	struct sockaddr_in server;
	struct hostent *hp;
	char buf[1024];
	char connectInp[BUFFERSIZE];
	char * token;
	int portno;
	
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) 
	{
		perror("opening stream socket");
		exit(0);
	}
	/* Connect socket using name specified by command line. */
	server.sin_family = AF_INET;
	//hp = gethostbyname(argv[1]);
	hp = gethostbyname("127.0.0.1");
	if (hp == 0) 
	{
		char buffer[BUFFERSIZE];
		write(STDOUT_FILENO, "127.0.0.1", strlen("127.0.0.1"));
		exit(0);
	}
	bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
	//server.sin_port = htons(atoi(argv[2]));
	server.sin_port = htons(2126);
	if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) 
	{
		perror("connecting stream socket");
		exit(1);
	}
	return sock;
}

void loop(int sock)
{
	char input[BUFFERSIZE];
	char * token;
	int msgsock;
	char buff[BUFFERSIZE];
	char sendClient[BUFFERSIZE];
	c ++;
	struct sockaddr_in cli_addr;
	listen(sock, BUFFERSIZE);
	int k = 0;
	int clilen;
	
	while(k < BUFFERSIZE)
	{
		
		struct threadData data;
		
		clilen = sizeof(cli_addr);
		
		msgsock = accept(sock, (struct sockaddr * ) &cli_addr, &clilen);
		
		if(msgsock == -1)
		{
			perror("Accept");
			
		}
		clientInfo[c].clientfd = msgsock;
		strcpy(clientInfo[c].ip, inet_ntoa(cli_addr.sin_addr.s_addr));
		clientInfo[c].portno = cli_addr.sin_port;
		
		
		
		
		data.fd = msgsock;
		
		pid_t acceptWorker = fork();
		
		if(acceptWorker > 0)     //server
		{
			int clientSock = clientCode();
			clientInfo[c].clientpid = acceptWorker;
			c++;
			pthread_t read_thread;
			int iret1;
			while(1)
			{
				iret1 = pthread_create(&read_thread, NULL, readThread, (void *) &clientSock);
			}
			
		}	
		
		if(acceptWorker == 0)			//child / subserver
		{
			int iret;
			pthread_t runThread, listThread, killThread;
			//
			int flag = 1;
			int serverSock = createSocket(flag);
			listen(serverSock, BUFFERSIZE);
			
			pthread_t write_thread;
			int sockz = accept(serverSock, NULL, NULL);
			
			while(1)
			{
		
				token = "sobia";
				int retval = read(msgsock, input, sizeof(input)-1);  		//reading from client
				if(retval == -1)
				{
					perror("Read on socket");
				}
				if(retval == 0)
				{
					write(STDOUT_FILENO, "Ending Connection\n", sizeof("Ending Connection\n")-1);
					exit(0);
				}				
				token = strtok(input," \n");
				
				
				iret = pthread_create(&write_thread, NULL, writeThread, (void *) sockz);
				
				
				
				if(strcasecmp(token,"run")==0) // run thread
				{
					
					token = strtok(NULL,"\n");
					strcpy(data.clientData, token);
					iret = pthread_create(&runThread, NULL, runProcess,(void *) &data);
					
				}
				if(strcasecmp(token,"exit")==0)
				{
					int j;
					for(j = 0; j < i; j++)
					{
						strcpy(ptr[j].status, "Terminated");
						ptr[j].end = clock();
						ptr[j].elapsedTime = ptr[j].end - ptr[j].start;
						int k = kill(ptr[j].pid, SIGKILL);
				
						if(k==-1)
						{
							perror("kill error");
						}
					}
					write(msgsock, token, strlen(token));
					
				}
				if(strcasecmp(token, "list")==0)
				{
					token = strtok(NULL, " \n");
					strcpy(data.clientData, token);
					data.fd = msgsock;
					iret =  pthread_create(&listThread, NULL, list,(void *) &data);			
				}
				if(strcasecmp(token, "kill")==0)
				{
					token =  strtok(NULL, " \n");
					strcpy(data.clientData, token);
					//write(STDOUT_FILENO, data.clientData, strlen(data.clientData));
					iret =  pthread_create(&killThread, NULL, killProcess,(void *) &data);
					if(iret)
					{
						char err[BUFFERSIZE];
						sprintf(err, "Error - pthread_create() return code: %d\n", iret);
						write(STDOUT_FILENO, err, strlen(err));
						exit(0);
					}
				}
				if(strcasecmp(token, "add")==0)
				{
				
					long sum = add(token);
					sprintf(sendClient, "The sum is %ld\n", sum);
					write(msgsock, sendClient, strlen(sendClient));
					
				}
				if(strcasecmp(token, "sub")==0)
				{
					long diff = sub(token);
					sprintf(sendClient, "The difference is %ld\n", diff);
					write(msgsock, sendClient, strlen(sendClient));
					
				}
				if(strcasecmp(token, "mul")==0)
				{
					
					long mult = mul(token);
					sprintf(sendClient, "The product is %ld\n", mult);
					write(msgsock, sendClient, strlen(sendClient));
					
				}
				if(strcasecmp(token, "div")==0)
				{
					long quotient = divide(token, msgsock);
					sprintf(sendClient, "The quotient is %ld\n", quotient);
					write(msgsock, sendClient, strlen(sendClient));
				}
				if(strcasecmp(token, "help")==0)
				{
					help(msgsock);
				}
				if(strcasecmp(token, "disconnect")==0)
				{
					write(STDOUT_FILENO,  "disconnect", sizeof("disconnect")-1);
					write(msgsock, "disconnect", sizeof("disconnect")-1);
					
				}
				if(strcasecmp(token, "client")==0)
				{
					int s;
					write(STDOUT_FILENO, "In client\n", sizeof("In client\n")-1);
					for(s = 0; s <= c; s++)
					{
						sprintf(sendClient, "%s, %d\n",clientInfo[s].ip, clientInfo[s].portno);
						write(STDOUT_FILENO, sendClient, strlen(sendClient));
					}
				}
				/*char buffer[BUFFERSIZE];
				int s; 
				for(s = 0; s <= c; s++)
				{
					
					sprintf(buffer,"%s, %d, %d\n", inet_ntoa(clientInfo[c].ip), clientInfo[c].portno, clientInfo[c].clientfd);
					write(STDOUT_FILENO, buffer, strlen(buffer));
				}*/
				
				
				write(msgsock,"",sizeof(""));
				
			}
		}
		
		else
		{
			write(msgsock, "Cannot Connect\n", sizeof("Cannot Connect\n")-1);
			write(msgsock, "exit", sizeof("exit")-1);
		}
		
		write(STDOUT_FILENO, "HI",2);
		
		k++;
	}
	
	
	
	
	
}

void * readThread(void * sock)			//server
{
	int serverSock = (int) sock;
	char buffer[BUFFERSIZE];
	read(STDIN_FILENO,buffer, sizeof(buffer)-1);
	write(serverSock, buffer, strlen(buffer));
	
	
}

void * writeThread(void * sock)			//subserver
{
	char buffer[BUFFERSIZE];
	int serverSock = (int) sock;
	read(serverSock, buffer, strlen(buffer));
	write(STDOUT_FILENO, buffer, strlen(buffer));
	if(strcasecmp(buffer, "list")==0)
	{
		struct threadData data;
		data.fd = serverSock;
		write(STDOUT_FILENO, "In run write thread\n", sizeof("In run write thread\n")-1);
		list((void *) &data);
	}
	if(strcasecmp(buffer, "run")==0)
	{
		
	}
}



	
long add(char * token)
{
	long sum = 0;
	long number = 0;
	char b[BUFFERSIZE];
	token = strtok(NULL, " ");
	
	while( token != NULL)
	{			
		number = atol(token);
		sum = sum + number;
		token = strtok(NULL, " ");
		write(STDOUT_FILENO, "In add", sizeof("In add")-1);
	}
	token = "sobia";
	return sum;
	
}


long sub(char * token)
{
	long diff = 0;
	long number = 0;
	char b[BUFFERSIZE];
	
	token = strtok(NULL, " ");
	while(token != NULL)
	{
		number = atol(token);	
		if(diff == 0)
		{
			diff = number - diff;
		}
		else
		{
			diff = diff - number;
		}
		token = strtok(NULL, " ");
	}
	write(STDOUT_FILENO,"The difference is: ",sizeof("The difference is: ")-1);
	sprintf(b, "%ld\n",diff);
	token = "sobia";
	return diff;
}

long mul(char * token)
{
	long mult = 1;
	long number = 1;	
	char b[BUFFERSIZE];
	token = strtok(NULL, " \n");
	
	while(token != NULL)
	{
		number = atol(token);
		mult = mult * number;
				
		token = strtok(NULL, " \n");
	}
	write(STDOUT_FILENO,"The product is: ",sizeof("The product is: ")-1);
	sprintf(b, "%ld\n",mult);
	token = "sobia";
	return mult;
}

long divide(char * token, int msgsock)
{
	token = strtok(NULL," ");
	char b[BUFFERSIZE];
	long number1 = 0;
	long number2 = 0;
	long frac = 1;
	number1 = atol(token);
	token = strtok(NULL, " ");
	number2 = atol(token);
	if(number2 == 0)
	{
			write(STDOUT_FILENO,"Infinite result\n",sizeof("Infinite result\n")-1);
			write(msgsock,"Infinite result\n",sizeof("Infinite result\n")-1);
	}
	else
	{
			frac = number1/number2;
			write(STDOUT_FILENO,"The quotient is: ",sizeof("The quotient is: ")-1);
			int count=sprintf(b, "%ld\n",frac);
			write(STDOUT_FILENO,b,count);
	}
	return frac;
}

void help(int msgsock)
{
	int opfd = open("help.txt", O_RDONLY);
	if(opfd == -1)
	{
		perror("Open error");
	}
	char help[2048];
	int re = 1;	
	re = read(opfd, help, 2048);
	write(msgsock, help, re);
	
}	
	
void * runProcess(void * ptr1)
{
	struct threadData *data = (struct threadData *) ptr1;
	char token[BUFFERSIZE];
	strcpy(token, (*data).clientData);
	int msgsock = (*data).fd;
	char sendClient[BUFFERSIZE];
	pid_t pid = fork();
	if(pid == 0)
	{
		int exe = execlp(token,token,NULL);
		if(exe == -1)
		{
			perror("Exec");
			write(msgsock, "Error in Exec\n", sizeof("Error in exec\n")-1);
		}
		kill(getppid(), SIGUSR1);
	}
	if(pid == -1)
	{
		perror("Fork");
	}
	if(pid > 0)
	{
		sleep(1);
		if(!executed)
		{
			//update tables
			ptr[i].pid=pid;
			strcpy(ptr[i].processName, token );			
			strcpy(ptr[i].status, "Active");
			ptr[i].start = clock();
			ptr[i].end = 0;
			ptr[i].elapsedTime = ptr[i].start;
			sprintf(sendClient, "%s is running\n", ptr[i].processName);
			i++;
			write(msgsock, sendClient, strlen(sendClient));	//writing on client fd
		}
		executed = 0;		
	}
}


void * list(void * ptr1)
{
	int j=0; int k = 0;
	char buff[BUFFERSIZE];
	char token[BUFFERSIZE];
	
	struct threadData *data = (struct threadData *) ptr1;
	strcpy(token, (*data).clientData);
	int msgsock = (*data).fd;
	for(j = 0; j < i; j++)
	{
		if(strcasecmp(ptr[j].status, "Terminated")==0)
		{
			k++;
		}
	}
	if(k == i-1 )
	{
		//write(STDOUT_FILENO, "No running processes\n", sizeof("No running processes\n")-1);
		write(msgsock, "No running processes\n", sizeof("No running processes\n")-1);
	}
	if(strcasecmp(token , "all")==0) //list all
	{
		//write(msgsock,"List all\n", strlen("List all\n"));
		//write(STDOUT_FILENO,"|ProcessName|PID|Start Time|Status|End Time|Elapsed Time|\n",sizeof("|ProcessName|PID|Start Time|Status|End Time|Elapsed Time|\n")-1);
		write(msgsock,"|ProcessName|PID|Start Time|Status|End Time|Elapsed Time|\n",sizeof("|ProcessName|PID|Start Time|Status|End Time|Elapsed Time|\n")-1);
		
		for(j=0; j < i; j++)
		{
			//read(msgsock, buff, strlen(buff));
			int s = sprintf(buff,"%s\t %d\t %ld\t %s\t %ld\t %ld\n",ptr[j].processName,ptr[j].pid, ptr[j].start, ptr[j].status, ptr[j].end, ptr[j].elapsedTime);
			if( s == -1)
			{
				perror("sprintf");
			}
			
			//write(STDOUT_FILENO, buff, strlen(buff));
			write(msgsock,buff, strlen(buff));		
		}
		
	}
	else				//list
	{
		//write(STDOUT_FILENO,"|ProcessName|PID|Start Time|Status|\n",sizeof("|ProcessName|PID|Start Time|Status|\n")-1);
		//write(msgsock,"List\n", strlen("List\n"));
		write(msgsock,"|ProcessName|PID|Start Time|Status|\n",sizeof("|ProcessName|PID|Start Time|Status|\n")-1);
		for(j=0; j < i; j++)
		{
			if(strcasecmp(ptr[j].status, "Active")==0)
			{
				//read(msgsock, buff, strlen(buff));
				int l = sprintf(buff,"%s\t %d\t %ld\t %s\n",ptr[j].processName,ptr[j].pid, ptr[j].start, ptr[j].status);
				if( l == -1)
				{
					perror("sprintf");
				}
				
				//write(STDOUT_FILENO, buff, strlen(buff));
				write(msgsock,buff, strlen(buff));
				
			}
			
		}
	}
}



void * killProcess(void * ptr1)
{
	
	char sendClient[BUFFERSIZE];
	char b[BUFFERSIZE];
	char *token;
	
	struct threadData *data = (struct threadData *) ptr1;
	strcpy(b, (*data).clientData);
	token = &b;
	int a = atoi(token);
	//write(STDOUT_FILENO, token, strlen(token));
	int msgsock = (*data).fd;
	int j=0;
	
	if(a == 0)
	{
		if(strcasecmp(token,"all")==0) //kill all
		{
			for(j = 0; j < i; j++)
			{
				strcpy(ptr[j].status, "Terminated");
				ptr[j].end = clock();
				ptr[j].elapsedTime = ptr[j].end - ptr[j].start;
				int k = kill(ptr[j].pid, SIGKILL);
				
				if(k==-1)
				{
					perror("kill error");
					write(msgsock, "Kill error\n",strlen("Kill error\n"));
				}
			}
			write(msgsock, "All processes are killed.\n",strlen("All processes are killed.\n")); //write on client fd
		}
		else 					//kill by name
		{
			for(j = 0; j < i; j++) 
			{
				if(strcasecmp(ptr[j].processName, token)==0)
				{
				//tokenize
				//if(token == null
					
					write(STDOUT_FILENO, "Kill name", strlen("Kill name"));
					token = strtok(NULL, " \n");
					/*int error = write(STDOUT_FILENO, token, strlen(token));
					if (error == -1 )
					{
						perror("Error in write");
					}*/
					
					if(strcasecmp(token, "all")==0)	//kill by name all
					{
				
						write(STDOUT_FILENO, "Kill name all", strlen("Kill name all"));
						for(j = 0; j < i; j++)
						{
							strcpy(ptr[j].status, "Terminated");
							ptr[j].end = clock();
							ptr[j].elapsedTime = ptr[j].end - ptr[j].start;
							int k = kill(ptr[j].pid, SIGKILL);
							if(k == -1)
							{
								perror("Kill in name all");
								write(msgsock, "Error in kill all.\n",strlen("Error in kill all.\n"));							}
						}
						sprintf(sendClient, "Processes of name %s are killed.\n", token);
						write(msgsock, sendClient, strlen(sendClient));
						break;
					}
					else		//kill by name
					{
						write(STDOUT_FILENO, "Kill application", strlen("Kill application"));
						if(strcasecmp(ptr[j].status, "Active")==0)
						{
							strcpy(ptr[j].status, "Terminated");
							ptr[j].end = clock();
							ptr[j].elapsedTime = ptr[j].end - ptr[j].start;
							int k = kill(ptr[j].pid, SIGKILL);
							if(k == -1)
							{
								perror("Kill in name");
								write(msgsock, "Error in kill name.\n",strlen("Error in kill name.\n"));
							}
							sprintf(sendClient, "%s killed.\n", ptr[j].processName);
							write(msgsock, sendClient, strlen(sendClient));
							break;
						}
					}
					
				}
			}
		}
	}
	else 									//kill by pid 
	{
		for(j = 0; j < i ; j++)
		{
			if( ptr[j].pid == a)
			{
				strcpy(ptr[j].status, "Terminated");
				ptr[j].end = clock();
				ptr[j].elapsedTime = ptr[j].end - ptr[j].start;
				break;
						
			}
		}
		int k = kill(a,SIGKILL);
		if(k==-1)
		{
			perror("kill error");
			write(msgsock, "Error in kill PID.\n",strlen("Error in kill PID.\n"));
		}
		sprintf(sendClient, "Process with PID %d is killed.\n", a);
		write(msgsock, sendClient, strlen(sendClient));
	}
}


