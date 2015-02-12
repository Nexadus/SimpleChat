#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>  
#include <sysexits.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <sys/select.h> 
#include <unistd.h>
#include <strings.h>
#include <pthread.h> 

#define MAX_INPUT 255
#define MAX_MESSAGES 8
#define NUM_ARGS 3
#define MAX_NAME 30
#define MAX_CLIENTS 5

void fillAddress(struct sockaddr_in* addr, int port, char* ip);

//struct with MsgQueue mutex
struct MessageQ{
	char messages[MAX_MESSAGES][MAX_INPUT];
	int FrontIndex;
	int BackIndex;
	int messagesInQ;
	pthread_mutex_t lock;
	pthread_cond_t empty;
	pthread_cond_t full;
	
};
//struct with ClientQue Mutex
struct ClientQ{
	int clientsfd[MAX_CLIENTS];
	int clients;
	int listenerFd;
	pthread_mutex_t lock;
	pthread_cond_t full;
	struct MessageQ* messages;
};
//Listener
struct ClientListener{
	int clientFD;
	int* clients;
	struct MessageQ* messages;
	
};
void initMessageQ(struct MessageQ* q);
void initClientQ(struct ClientQ* q, int fd, struct MessageQ* mess);
void addMessage(struct MessageQ* q, char* msg, int numread);
char* removeMessage(struct MessageQ* q);
void writeAll(char * mes, struct ClientQ* c);


void* listenClients(void* p)
{
	
	char buffer[MAX_INPUT];
	bzero(buffer,MAX_INPUT);
	struct ClientListener* client= (struct ClientListener*) p;
	printf("starting client\n listening for %d\n", client->clientFD);
	while(1)
	{
		int numread=read(client->clientFD, buffer, MAX_INPUT);
		if(numread==0)
		{
			
		}
		puts("message recieved");
		if(numread!=30)
		addMessage(client->messages,buffer,numread);
		//puts("hello");
		bzero(buffer,numread);
	}
};
struct MessageClientQ{
	struct MessageQ * m;
	struct ClientQ * c;
};
void* messageSender(void* p)
{
	puts("writer ready");
	struct MessageClientQ* q= (struct MessageClientQ*) p;
	while(1)
	{
		if(q->m->messagesInQ>0)
		{
			puts("printing message");
			char* mes;
			mes= removeMessage(q->m);
			printf("%s\n",mes);
			if(mes!=NULL)
				writeAll(mes, q->c);
			free(mes);
		}
	}
}
void* recieveCLients(void* p)
{
	struct ClientQ* q=(struct ClientQ*) p;
	while(1)
	{	
		if (listen( q->listenerFd, MAX_CLIENTS-1 /* only want 1 client*/ ) == -1 )
		{
			perror("server failed to listen.");
			exit(5);
		}
		
		while(q->clients >= MAX_CLIENTS)
		{
			pthread_cond_wait(&q->full, &q->lock);
		}
		int connect_fd=0;
		
		if ((connect_fd = accept(q->listenerFd, (struct sockaddr*) NULL, NULL)) == -1){
			printf("listening on fd %d\n",q->listenerFd);
			perror("server failed to accept.");
			//exit(25);//TO BE REMOVED
		}
		pthread_mutex_lock(&q->lock);
		puts("ClientQ locked:recieveCLients");
		printf("client fd created: %d\n",connect_fd);
		for(int i=0; i<MAX_CLIENTS;i++)
		{
			if(q->clientsfd[i]==0)
			{
				//puts("in if");
				q->clientsfd[i]=connect_fd;
				//printf("%d : client added\n",q->clientsfd[i]);
				struct ClientListener x;
				x.clientFD=connect_fd;
				x.clients=q->clientsfd;
				x.messages=q->messages;
				pthread_t clientListen;
				pthread_create(&clientListen,NULL,listenClients,&x);
				break;
			}
			//else printf("%d in spot %d of array",q->clientsfd[i],i);
		}
		pthread_mutex_unlock(&q->lock);
		puts("ClientQ unlocked:recieveCLients");
	}
	return 0;
};

int main( int argc, char* argv[] ) {
	int port;
	//ERROR CODE 1
	if ( argc != NUM_ARGS ) {
		perror("usage: ./Server PORT IP");
		exit(1);
	}
	port= atoi(argv[1]);
	//ERROR CODE 2
	if(port<0)
	{
		perror("invalid port");
		exit(2);
	}
	int listen_fd;
	struct sockaddr_in sin;//address
	fillAddress(&sin, port, argv[2]);
	//ERROR CODE 3
	if ( (listen_fd = socket( AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		perror("server failed to socket.");
		exit(3);
	}
	//ERROR CODE 4
	if (bind( listen_fd, (struct sockaddr*) &sin, sizeof(sin)) == -1)
	{
		perror("server failed to bind.");
		exit(4);
	}
	puts("listening");
	pthread_t Listener;
	struct ClientQ clientQueue;
	struct MessageQ messageQueue;
	initMessageQ(&messageQueue);
	initClientQ(&clientQueue,listen_fd,&messageQueue);
	pthread_create(&Listener, NULL, recieveCLients,&clientQueue);
	//incoming connections handled
	pthread_t writer;
	struct MessageClientQ bothq;
	bothq.m=&messageQueue;
	bothq.c=&clientQueue;
	pthread_create(&writer,NULL, messageSender, &bothq);
	//Run forever listening to clients, recieving, knowing when they write something and leave the chat server
	while(1)
	{
	}
	
	//Program finishes
	exit(0);
}

void fillAddress(struct sockaddr_in* addr, int port, char* ip)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	inet_pton(AF_INET, ip, &(addr->sin_addr));
};
void initMessageQ(struct MessageQ* q)
{
	q->FrontIndex=0;
	q->BackIndex=0;
	q->messagesInQ=0;
	for(int i=0; i< MAX_MESSAGES;i++)
		bzero(q->messages[i],MAX_INPUT);
	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->empty,NULL);
	pthread_cond_init(&q->full,NULL);
};
void initClientQ(struct ClientQ* q, int fd, struct MessageQ* mess)
{
	q->clients=0;
	q->listenerFd=fd;
	q->messages=mess;
	for(int i=0; i< MAX_CLIENTS;i++)
		q->clientsfd[i]=0;
	pthread_mutex_init(&q->lock, NULL);
	pthread_cond_init(&q->full,NULL);
};
void addMessage(struct MessageQ* q, char* msg, int numread)
{
	//puts("adding message");
	pthread_mutex_lock(&q->lock);
	puts("MesssageQ locked:addMessage");
	while(q->messagesInQ>MAX_MESSAGES)
	{
		pthread_cond_wait(&q->full, &q->lock);
	}
	//puts("out of lock");
	pthread_cond_broadcast(&q->full);
	strncpy(q->messages[q->BackIndex],msg,numread);
	//printf("in add message, message recieved :%s\n",msg);
	q->BackIndex++;
	if(q->BackIndex>4)
		q->BackIndex-=5;
	q->messagesInQ++;
	pthread_mutex_unlock(&q->lock);
	puts("MessageQ unlocked:add message");
};
char* removeMessage(struct MessageQ* q)
{
	pthread_mutex_lock(&q->lock);
	puts("MessageQlocked:removeMessage");
	char* message=NULL;
	if(q->messagesInQ>0)
	{
		char* retval= (char*) malloc(MAX_INPUT);
		strncpy(retval, q->messages[q->FrontIndex],MAX_INPUT);
		q->messagesInQ-=1;
		q->FrontIndex+=1;
		if(q->FrontIndex>4)
			q->FrontIndex-=5;
		message=retval;
	}
	pthread_mutex_unlock(&q->lock);
	puts("MessageQ unlocked:removeMessage");
	//printf("returning message : %s\n",message);
	return message;
}
void writeAll(char * mes, struct ClientQ* c)
{
	puts("in write all method");
	pthread_mutex_lock(&c->lock);
	puts("ClientQ locked:writeAll");
	printf("writing to all: %s\n",mes);
	for(int i=0; i<MAX_CLIENTS; i++)
	{
		printf("c->clientsfd[i] = %d\n",c->clientsfd[i]);
		if(c->clientsfd[i]!=0)
		{
			int x= write(c->clientsfd[i], mes, MAX_INPUT);
			printf("results of writing to %d, string %s: %d\n",c->clientsfd[i],mes,x);
		}
	}
	pthread_mutex_unlock(&c->lock);
	puts("ClientQ unlocked:writeAll");
}
