// Server side C/C++ program to demonstrate Socket programming
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#define PORT 8080
#define AUTH_TRIES 20
#define CONN_SUCCESS "0"
#define AUTH_FAIL_0 "1"
#define AUTH_FAIL_1 "2"
#define SEND_FAIL "3"
#define SEND_FAIL_LONG "Message integrity check failed. Try again.\n"
#define REALLOC_ERR "Could not reallocate memory for new user\n"
#define TRIES_ERR "A user could not connect(MAX TRIES EXCEEDED)\n"

char* itoa(int val, int base){						//int to array function(borrowed)
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
	
}

void addLength(char* string)						//function that adds the length of a string at its end
{
	char buffer[10];
	int length = strlen(string);
	strcpy(buffer,itoa(length,10));
	strcat(string," ");
	strcat(string,buffer);
}

void remLength(char* string)						//function that removes the length from the end of a string
{
	(strrchr(string,' '))[0]='\0';
}

int checkLength(char *string)						//function that checks that a string has the declared length +remLength
{
	int length = atoi(strrchr(string,' ')+1);
	remLength(string);
	return strlen(string)==length;
}

struct user											//user name+socket structure
{
	char name[32];
	int socket;
}*users;
int usercount;
int server_fd;
int addrlen;
struct sockaddr_in address;

void remove_client(char* remName)					//function that removes client by name
{
	int i=0;
	while(strcmp(users[i].name,remName)!=0) i++;
	for(;i<usercount-1;i++)
		users[i]=users[i+1];
	usercount--;
}

void* user_thread(void* args)
{
	int socket = ((int*)args)[0];
	int willAdd=0,valread;
	char buffer[1024],buffer2[1024];
	struct user user; 
	struct user* aux;
	int authTries=AUTH_TRIES; 						//constant - every user gets 20 tries to connect
	//add user's name
	while(!willAdd)
	{		
		willAdd=1;
		valread = read( socket , buffer, 1024);		//server reads the name request
		if(valread<1)
		{
			return 0;
		}
		if((--authTries)<0) 						// server automatically disconnects user after 20 failed connection attempts
		{	
			printf(TRIES_ERR);
			send(socket , AUTH_FAIL_0 , 2 , 0 );	// auth tries exceeded fail
			return 0;
		}	
		if(!checkLength(buffer))					// server checks the integrity of the recieved name
		{
			send(socket , SEND_FAIL , 2 , 0 );		// letter check fail
			willAdd=0;
			continue;
		}
		for(int i=0;i<usercount;i++)				// server checks the availability of the name
			if(strcmp(users[i].name,buffer)==0)
			{
				willAdd=0;break;
			}
		
		if(willAdd)									// server tries to add user to the user table
		{
			strcpy(user.name,buffer);
			user.socket=socket;
			
			aux=(struct user*)realloc(users,++usercount*sizeof(struct user));
			if(aux==NULL)
			{
				printf(REALLOC_ERR);
				send(socket , AUTH_FAIL_0 , 2 , 0 );//mem realloc fail
			}
			else
			{
				aux[usercount-1]=user;
				users=aux;
				printf("USER %s CONNECTED\n",buffer);

				send(socket , CONN_SUCCESS , 2 , 0 );
			}
		}
		else
		{
			send(socket , AUTH_FAIL_1 , 2 , 0 );//used nickname fail
		}
	}
	//get requests from that user
	while(1)
	{
		valread = read( socket , buffer, 1024);				//get requests from user
		if(valread<1)
		{
			printf("USER %s DISCONNECTED\n",user.name);
			remove_client(user.name);
			return 0;
		}
		if(strcmp(buffer,"/LIST")==0)						//if the user asks for user list
		{
			strcpy(buffer,"\n\tConnected users:\n\t");		//it is formatted
			for(int i=0;i<usercount;i++)
			{
				strcat(buffer,users[i].name);
				strcat(buffer,"\n\t");
			}
			strcat(buffer,"\n");
			send(socket, buffer, strlen(buffer)+1, 0);		//and sent
		}
		else												//else
		{
			if(checkLength(buffer)){						//if the message is complete
				strcpy(buffer2,"---");						//the message is formatted to be sent to everyone
				strcat(buffer2,user.name);
				strcat(buffer2,": ");
				strcat(buffer2,buffer);
				strcat(buffer2,"\n");
				printf("%s",buffer2);
				for(int i=0;i<usercount;i++)				//and sent to all users(except the sender)
				{
					if(strcmp(users[i].name,user.name)!=0)
						send(users[i].socket,buffer2,strlen(buffer2)+1,0);
				}
			}
			else send(socket, SEND_FAIL_LONG, 45 , 0);
		}
	}
}



int main(int argc, char const *argv[])
{
	users = (struct user*) malloc ( 0 );
	int new_socket;
    int opt = 1;
    addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    // Forcefully attaching socket to the port 808041
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 50) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
	
	while(1)
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
		{
			perror("accept");
			exit(EXIT_FAILURE);
		}
		pthread_t th2;
		pthread_create( &th2, NULL, user_thread, &new_socket);	//create the thread that handles the user's auth and messaging
	}
	
    return 0;
}