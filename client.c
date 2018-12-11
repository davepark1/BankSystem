#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <netdb.h>
#include "bank.h"
#include <time.h>
#define MAX_LIMIT 20
char messageSent[MAX_LIMIT]; 
int sock = 0, valread; 

int PORT;
void* inputthread()
{
  while(1)
    {
      // printf("begin\n");
      fgets(messageSent,MAX_LIMIT,stdin);
      send(sock , messageSent , strlen(messageSent) , 0 ); 
    }
}

void *outputthread()
{
  
  while(1)
    {
      char buffer[1024] = {0}; 
      valread = read( sock , buffer, 1024); 
      printf("%s\n",buffer ); 
      if(strcmp(buffer,"terminate")==0)
	{
	  printf("session has been ended\n");  
	exit(0);
	}
    }
}

int main(int argc, char *argv[]) 
{ 

  sscanf(argv[2],"%d",&PORT);
    struct hostent *host;
    struct sockaddr_in address; 
    pthread_t inputthread_id,outputthread_id;
    struct sockaddr_in serv_addr; 
    
    
    char* hostname= (strdup(argv[1]));
    printf("%d\n",PORT);
    printf("%s\n",hostname);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    } 
   
    memset(&serv_addr, '0', sizeof(serv_addr)); 

    
    
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    host = gethostbyname(hostname);
    memcpy(&serv_addr.sin_addr,host->h_addr,host->h_length);
    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)  
    { 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    } 
    while(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
    { 
      sleep(3);
      printf("here\n");
    } 
    printf("Connected to server\n");
    pthread_create(&inputthread_id,NULL,inputthread,NULL);
    pthread_create(&outputthread_id,NULL,outputthread,NULL);
    pthread_join(inputthread_id,NULL);
    pthread_join(outputthread_id,NULL);
    exit(0); 
} 
