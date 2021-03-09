// Client side C/C++ program to demonstrate Socket programming 
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <pthread.h> 
#include <unistd.h> 
#include <string.h> 
#define PORT 8080 
#define CONN_SUCCESS "0"
#define AUTH_FAIL_0 "1"
#define AUTH_FAIL_1 "2"
#define SEND_FAIL "3"

char* itoa(int val, int base){
	static char buf[32] = {0};
	int i = 30;
	for(; val && i ; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
	return &buf[i+1];
}

void addLength(char* string)
{
	char buffer[10];
	int length = strlen(string);
	strcpy(buffer,itoa(length,10));
	strcat(string," ");
	strcat(string,buffer);
}

void remLength(char* string)
{
	(strrchr(string,' '))[0]='\0';
}

int checkLength(char *string)
{
	int length = atoi(strrchr(string,' ')+1);
	remLength(string);
	return strlen(string)==length;
}

void* thrd_read_from_srv(void* args)
{
	int valread;
	int socket = ((int*)args)[0];
	char buffer[1024];
	while(1)
	{
		valread = read( socket , buffer, 1024);
		if(valread<1)
		{
			printf("MURIT SERVER NU PLANGE\n");
			exit(0);
		}
		printf("%s",buffer);
	}
}
   
int main(int argc, char const *argv[]) 
{ 

	
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    char name[100]; 
    char buffer[1024] = ""; 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
   
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
        printf("\nCould not connect. Try again later. \n"); 
        return -1; 
    } 
	int connected=0;
	while(!connected)
	{
		int tries=5;
		printf("NICKNAME:");
		fgets(name,100,stdin);
		name[strlen(name)-1]=0;
		addLength(name);
		send(sock , name , strlen(name)+1 , 0 ); 
CHECK:																		//CHECK- tag for re-reading response in case of packet loss
		valread = read( sock , buffer, 1024);
		if(valread<1)
		{
			printf("No response from server\n");
			exit(0);
		}
		if(strcmp(buffer,SEND_FAIL)==0){									//SEND_FAIL = message integrity check failed
			if((--tries)>0)
			{
				send(sock , name , strlen(name)+1 , 0 ); 					//send name again
				goto CHECK;													//go back to CHECK tag
			}
			else continue;
		}
		if(strcmp(buffer,AUTH_FAIL_0)==0){									//AUTH_FAIL_0 = connection failed
			printf("Could not connect. Try again later.\n");
			exit(0);
		}
		if(strcmp(buffer,AUTH_FAIL_1)==0)									//AUTH_FAIL_1 = name already used
			printf("Nickname already used. Try again.\n");
		if(strcmp(buffer,CONN_SUCCESS)==0)
		{
			printf("Connection successful.\n");
			connected=1;
		}
	}		
	
	pthread_t th2;
	pthread_create( &th2, NULL, thrd_read_from_srv, &sock);
	
	while(1)
	{
		fgets(buffer,100,stdin);
		buffer[strlen(buffer)-1]=0;
		if(buffer[0]=='/')
			if(strcmp(buffer,"/LIST")==0)
				send(sock,buffer,strlen(buffer)+1,0);
			else
			{
				if(strcmp(buffer,"/LEAVE")==0)
					exit(0);
				else
					printf("USABLE COMMANDS: /LIST /LEAVE\n");
			}
		else
		{
			addLength(buffer);
			send(sock,buffer,strlen(buffer)+1,0);
		}
	}
	
    return 0; 
} 