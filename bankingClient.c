#include "banking.h"


Acct* currentAcct = NULL; //current account logged into. if NULL, no one is logged in.
int clientSocket;

//Read messages from server and send them to user
void* responseOutput(void* arg){
	//This will be a different thread constantly listening to the server
	char buffer[1024];
	int valread;
	
	while(1){
      valread = read( sock , buffer, 1024); 
      printf("%s\n",buffer );
      
      if(strcmp(buffer,"terminate")==0)
		exit(0);
    }
}



int main (int argc, char** argv){
	
	if (argc != 3){
		printf("Incorrect input format. Correct format: ./bankingClient [machine name] [port number]\n");
		exit(0);
	}
	
	char* machine = argv[1];
	int port = atoi(argv[2]);

	struct hostent* host = gethostbyname(machine);
	
	if (host == NULL){
		printf("Could not get address of machine.\n");
		exit(0);
	}
	
	int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (clientSocket < 0){
		printf("Error in creating socket.\n");
		exit(0);
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	
	memcpy(&addr.sin_addr, host->h_addr, host->h_length);
	
	if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0){
		printf("Invalid address/Address not supported\n");
		exit(0);	
	}
	
	printf("(1) Attempting to connect...\n");
	while(connect(clientSocket, (struct sockaddr*) &addr, sizeof(addr)) < 0){
		printf("(1) Retrying...\n");
		sleep(3);
	}
	printf("(1) Connection successful.\n");
	
	
	pthread_t thread;
	pthread_create(&thread, NULL, responseOutput, NULL);
	
	
	char buffer[1024] = "";
	
	while(1){

		char* cmd[2];
		cmd[0] = NULL;
		cmd[1] = NULL;
		
		char* token;
		char* ptr;
		int pos = 0;
		
		printf("Enter command:\n");
		scanf("%s", buffer);
		
		ptr = buffer;
		while((token = strtok_r(ptr, " ", &ptr))){
			cmd[pos] = (char*) malloc(sizeof(char)*(strlen(token)+1));
			strcpy(cmd[pos], token);
			pos++;
		}
		
		//  *NOTE* : need to work out format of TCP message.
		if (strcmp(cmd[0], "create") == 0){ 
			printf("In create\n");
			Acct* newAcct = (Acct*) malloc(sizeof(Acct));
			newAcct->name = cmd[1];
			
		}else if(strcmp(cmd[0], "serve") == 0){
			printf("In serve\n");
			
		}else if(strcmp(cmd[0], "deposit") == 0){
			printf("In deposit\n");
			if (currentAcct == NULL){
				printf("User is not yet logged in.\n");
				sleep(2);
				continue;
			}
		}else if(strcmp(cmd[0], "withdraw") == 0){
			printf("In withdraw\n");
			if (currentAcct == NULL){
				printf("User is not yet logged in.\n");
				sleep(2);
				continue;
			}
			
		}else if(strcmp(cmd[0], "query") == 0){
			printf("In query\n");
			
		}else if(strcmp(cmd[0], "end") == 0){
			printf("In end\n");
			
		}else if(strcmp(cmd[0], "quit") == 0){
			printf("Quitting...\n");
			break;
		}else{
			printf("Invalid command. Try again\n");
		}
		
		if (cmd[0] != NULL){
			free(cmd[0]);
		}
		if (cmd[1] != NULL){
			free(cmd[1]);
		}
		
		sleep(2);
	}
	
	close(clientSocket);	
	
	return 0;
}