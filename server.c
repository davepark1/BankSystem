#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include "bank.h"
account* accountList;//List of all accounts in the bank
userinfo *userarray;//for keeping track of the threads
int totalaccounts = 0;   
pthread_t *threads;//all threads
pthread_mutex_t lock;//locks
int threadIndex = 0;

void *clientthread();
void sighandle(int signo);
void create(char* accname,int new_socket);
int serve(char *accname);
void printsig();
void deposit(int accnum, double moneyexchange);
void withdraw(int accnum, double moneyexchange);


void main(int argc, char *argv[])
{
  int PORT;
  int new_socket;
  clock_t start = clock();
  pthread_mutex_init(&lock,NULL);
  threads = (pthread_t *)malloc(sizeof(pthread_t)*1024);

  userarray= (struct userinfo*)malloc(sizeof(userinfo)*1024);
  accountList= (struct account*)malloc(sizeof(account)*1024);//malloc space for accounts and users

  sscanf(argv[1],"%d",&PORT);//cast argv[1] to int for PORT

  signal(SIGINT, sighandle);//==SIG_ERR;//declare signal
  signal(SIGALRM, sighandle);
  alarm(15);//initial alarm declaration
  
  int server_fd; //initial declarations and error checks
  struct sockaddr_in address; 
  int opt = 1; 
  int addrlen = sizeof(address); 
  char buffer[1024] = {0}; 
       
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    }   
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 

  address.sin_family = AF_INET; //for correct protocol
  address.sin_addr.s_addr = INADDR_ANY;//because we are going to be on a local connection 
  address.sin_port = htons( PORT ); //for the correct port
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) //binds to prev criteria
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
  if (listen(server_fd, 3) < 0) //waits for connection but queue may fill
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 

  while(new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))
      {
	printf("here too\n");
	if(new_socket<0) 
	  { //takes first connection off queue if <0 it is incorrect
	    perror("accept"); 
	    exit(EXIT_FAILURE); 
	  } 
	else
	  {
	    
	    userinfo tempinfo;
	    tempinfo.newsocket = new_socket;
	    userarray[threadIndex]=tempinfo;
	    pthread_create(&threads[threadIndex],NULL,clientthread,&userarray[threadIndex]);
	    threadIndex +=1;
	    
	  }
      }
    
    
}

// Place for the client thread to go will add in more so it make send in commands
void* clientthread(void *temp)
{
  userinfo *per = (userinfo*)temp;
  int new_socket = per->newsocket;
  int valread;
  char buffer[1024] = {0};
  char* response=NULL;
  char *firstword;
  char *rest;
  char tempbalance[60];
  int accnum = -1;
  double moneyexchange;
  while(1)
    {
      response= (char*)malloc(sizeof(char)*30);
    valread = read(new_socket , buffer, 1024); 
    if(valread ==0)//if valread = 0 then client has terminated
      break;
    firstword = strtok(buffer," ");
    rest = strtok(NULL,"\n");
    if(strcmp(firstword,"create")==0)
      {
	//printf("printing %s\n",rest);
	create(rest,new_socket);
	//strcpy(response,"string has been created");
      }
    else if(strcmp(firstword,"serve")==0)
      {
	if(accnum != -1)
	  strcpy(response,"End session with current account first");
	else
	  {
	  accnum = serve(rest);
	  if(accnum == -1)
	    {
	      strcpy(response,"Account does not exist or is being used ");
	    }
	  else
	    {
	      strcpy(response,"now serving ");
	      strcat(response,rest);
	    }
	  }
      }
    else if(strcmp(firstword,"deposit")==0)
      {
	sscanf(rest,"%lf",&moneyexchange);
	if(accnum == -1)
	  strcpy(response,"first select and account");
	else if(moneyexchange <0)
	  strcpy(response,"deposit must be positive");
	else
	  {
	    deposit(accnum,moneyexchange);
	    strcpy(response,"deposit successful");
	  }
      }
    else if(strcmp(firstword,"withdraw")==0)
      {
	sscanf(rest,"%lf",&moneyexchange);
	if(accnum == -1)
	  strcpy(response,"first select and account");
	else if(moneyexchange <0)
	  strcpy(response,"withdraw must be positive");
	else
	  {
	    if(accountList[accnum].balance<moneyexchange)
	      strcpy(response,"withdraw must not be greater than balance");
	    else
	      {
		 withdraw(accnum,moneyexchange);
		 strcpy(response,"withdraw successful");
	      }
	  }
      }
    else if(strcmp(strtok(firstword,"\n"),"query")==0)
      {
	if(accnum == -1)
	  strcpy(response,"first select a account");
	else
	  {
	    strcpy(response,"Balance is ");
	    sprintf(tempbalance,"%0.2lf",accountList[accnum].balance);
	    strcat(response,tempbalance);
	  }
      }
    else if(strcmp(strtok(firstword,"\n"),"end")==0)
      {
	accnum = -1;
	strcpy(response,"Account session has been ended");
      }
    else if(strcmp(strtok(firstword,"\n"),"quit")==0)
      {
	accnum = -1;
	//strcpy(response,"Session has been ended\n");
	//send(new_socket , response , strlen(response), 0 ); 
	send(new_socket , "terminate" , 15 , 0 );
	break;
      }
    else
      {
	strcpy(response,firstword);
	strcat(response,"|");
	sprintf(tempbalance,"%d",strcmp(strtok(firstword,"\n"),"query"));
	strcat(response,tempbalance);
	strcat(response,"|");
      }

    send(new_socket , response , strlen(response), 0 ); 
    free(response);
    //printf("Hello message sent\n"); 
    }
  
}
// SIGINT signal handler for the code
void sighandle(int signo)
{
   if(signo == SIGINT)
	{
	  int count = 0;
	  
	  while(count<threadIndex)
	    {
	      userinfo temp = userarray[count];
	      int new_socket = temp.newsocket;
	      send(new_socket , "terminate" , 15 , 0 );
	      count++;
	    }
	  count = 0;
	   while(count<threadIndex)
	    {
	      pthread_join(threads[count],NULL);
	      count++;
	    }
	   exit(0);
	}
   else if(signo == SIGALRM)
     {
       // signal(SIGALRM,SIG_IGN);
       // printf("boop\n");//alarm must be deactivated then reactivated
       printsig();
       alarm(15);
       //signal(SIGALRM,sighandle);
     }
   
}
void printsig()
{
  pthread_mutex_lock(&lock);
  int count = 0;
  printf("Name:   \t");
  printf("Balance       \t");
  printf("Service\n");
  while(count<totalaccounts)
    {
      printf("Name: %s\t",accountList[count].name);
      printf("Balance: %0.2lf\t",accountList[count].balance);

      if(accountList[count].service == 1)//If in service say so
	printf("IN SERVICE\n");
      else
	printf("\n");
      count++;
    }
  printf("\n");
  pthread_mutex_unlock(&lock);
}

// Different commands from the user to the server
void create(char* accname,int new_socket)
{
  pthread_mutex_lock(&lock);
  int count = 0;
  while(count<totalaccounts)
    {
      if(strcmp(accountList[count].name,accname)==0)
	{
	  printf("Error:Account Exists");
	  pthread_mutex_unlock(&lock);
	  send(new_socket , "Error: Account Exists" , strlen("Error: Acount Exists") , 0 );
	  return;
	}
      count++;
    }
  account acc;
  acc.name = (char*)malloc(sizeof(accname));
  strcpy(acc.name,accname);
  acc.balance = 0;
  acc.service = 0;
  accountList[totalaccounts]= acc;
  totalaccounts += 1;
  pthread_mutex_unlock(&lock);
  send(new_socket , "Account created" , strlen("Account Created") , 0 );
  return;
}

int serve(char *accname)
{
  pthread_mutex_lock(&lock);
  int count = 0;
  account temp;
  while(count < totalaccounts)
    {
      temp = accountList[count];
      if(strcmp(accname,temp.name)==0)
	{
	  break;
	}
      count++;
    }
  if(count == totalaccounts)
    {
    printf("error\n");
    return -1;
    }
  if(accountList[count].service == 1)
    {
    printf("already being accessed");
    return -1;
    }
  accountList[count].service = 1;
  pthread_mutex_unlock(&lock);
  return count;
}
void deposit(int accnum, double moneyexchange)
{
  pthread_mutex_lock(&lock);
  accountList[accnum].balance += moneyexchange;
  pthread_mutex_unlock(&lock);
  return;
}
void withdraw(int accnum, double moneyexchange)
{
  pthread_mutex_lock(&lock);
  accountList[accnum].balance -= moneyexchange;
  pthread_mutex_unlock(&lock);
  return;
}

