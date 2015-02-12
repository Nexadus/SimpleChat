#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>  
#include <sysexits.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <sys/select.h> 
#include <unistd.h>
#include <strings.h>

#define MAX_INPUT 255
#define NUM_ARGS 4
#define MAX_NAME 30

void fillAddress(struct sockaddr_in* addr, int port, char* ip);

int main( int argc, char* argv[] ) {
	char* name;
	int port;
	//ERROR CODE 1
	if ( argc != 4) {
		perror("usage: ./Client USERNAME PORT IP");
		exit(1);
	}
	//Get the name and port number
	name=argv[1];
	port= atoi(argv[2]);
	//ERROR CODE 2
	if(port<0)
	{
		perror("invalid port");
		exit(2);
	}
	int connect_fd;	// socket descriptors
	struct sockaddr_in sin;//address
	fd_set fds; //file desriptor
	fillAddress(&sin, port, argv[3]);
	//ERROR CODE 3
	if ( (connect_fd = socket( AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		perror("server failed to socket.");
		exit(3);
	}
	//ERROR CODE 6
	if ( (connect(connect_fd, (struct sockaddr*) &sin,sizeof(sin))) == -1){
		perror("server failed to accept.");
		exit(6);
	}
	char buffer[MAX_INPUT];//input buffer
	puts("starting chat");
	bzero(buffer,sizeof(buffer));
	while(1)
	{
		FD_ZERO(&fds);
		FD_SET(0, &fds);//stdin
		FD_SET(connect_fd, &fds);//client socket
		select(connect_fd+1, &fds, NULL, NULL, NULL);
		if ( FD_ISSET(0, &fds) )
		{
			if (read(0,buffer, MAX_INPUT) ==0 )
			{
			perror("server failed to read keyboard");
			close(connect_fd);
			exit(7);
			}
			else
			{
				buffer[strlen(buffer)-1]='\0';//removes the new line from input
				//puts("keyboard detected");
				char output[MAX_INPUT];
				sprintf(output, "%s : %s",name, buffer);
				write(connect_fd,output,sizeof(output));
				//puts(output);
				bzero(buffer,sizeof(buffer));
			}
		}
		if ( FD_ISSET(connect_fd, &fds) )
		{
			int recieved;
			if((recieved=recv(connect_fd,buffer,sizeof(buffer),0))==0)
			{
				perror("failed to recieve");
				close(connect_fd);
				exit(8);
			}
			else
			{
				//puts("server detected");
				buffer[recieved-1]='\0';
				printf("%s\n",buffer);
				memset(buffer, 0, recieved);
			}
		}
	}
	exit(0);
}

void fillAddress(struct sockaddr_in* addr, int port, char* ip)
{
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	inet_pton(AF_INET, ip, &(addr->sin_addr));
}
