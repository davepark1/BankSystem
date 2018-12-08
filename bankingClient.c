#include "banking.h"


char* currentAcct = NULL; //current account logged into. if NULL, no one is logged in.
int clientSocket;
int status;  //1: active   0: inactive

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; //status

//Read messages from server and send them to user
void* responseOutput(void* arg){
	//This will be a different thread constantly listening to the server
	char buffer[1024];
	int valread;
	
	while(1){
      valread = read( clientSocket , buffer, 1024); 
      printf("%s\n",buffer );
      
      if(strcmp(buffer,"terminate")==0)
		pthread_exit(NULL);
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

	active = 1;
	
	
	char buffer[1024] = "";
	
	while(active == 1){

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
		
		//  *NOTE*  TCP message format (create, serve):   "commandNumber:messageLength:message"
		//  *NOTE*  TCP message format(deposit, withdraw): "commandNumber:messageLength:account,deposit/withdraw amount"
		//  ACK:   0: error   else ==> success

		char message[1050] = "";
		char tail[1043] = "";

		if (strcmp(cmd[0], "create") == 0 && cmd[1] != NULL){ 
			printf("In create\n");

			//create message
			//TCP MESSAGE FORMAT:  "messageLength:commandNumber:message"
			strcpy(message, "1:");

			char len[6];			//variable to hold length of message (account)
			sprintf(len, "%d", strlen(cmd[1]));   //convert length of message into string

			strcat(message, len);
			strcat(message, ":");
			strcat(message, cmd[0]);
			int ret = send(clientSocket, message, 1050, 0);   //might have to cast message to void*

			if (ret == -1){
				printf("Failed to send message to server (1)\n");
				continue;
			}

			//wait for ACK in case account already exists
			strcpy(buffer, "");
			printf("Waiting for ACK\n");
			while(strcmp(buffer, "") == 0){
				read(clientSocket, buffer, 1024);
			}
			printf("ACK received\n");

			if (strcmp(buffer, "0") != 0){
				printf("Account: %s  created\n", cmd[1]);
			}else{
				printf("ERROR: Account %s already exists\n", cmd[1]);
			}
			
		}else if(strcmp(cmd[0], "serve") == 0 && cmd[1] != NULL){
			printf("In serve\n");
			
			//create message
			//TCP MESSAGE FORMAT:  "messageLength:commandNumber:message"
			strcpy(message, "2:");

			char len[6];			//variable to hold length of message (account)
			sprintf(len, "%d", strlen(cmd[1]));   //convert length of message into string

			strcat(message, len);
			strcat(message, cmd[1]);
			int ret = send(clientSocket, message, 1050, 0);   //might have to cast message to void*

			if (ret == -1){
				printf("Failed to send message to server (2)\n");
				continue;
			}

			//Wait for ACK in case account doesn't exist
			strcpy(buffer, "");
			printf("Waiting for ACK\n");
			while(strcmp(buffer, "") == 0){
				read(clientSocket, buffer, 1024);
			}
			printf("ACK received\n");

			if (strcmp(buffer, "0") != 0){
				printf("Account: %s logged in successfully\n", cmd[1]);
				currentAcct = (char*) malloc(sizeof(char)*(strlen(cmd[1])+1));
				strcpy(currentAcct, cmd[1]);
			}else{
				printf("ERROR: Could not log into account %s\n", cmd[1]);
			}
			
		}else if(strcmp(cmd[0], "deposit") == 0 && cmd[1] != NULL){
			printf("In deposit\n");

			if (currentAcct == NULL){
				printf("User is not yet logged in.\n");
				free(cmd[0]);
				if (cmd[1] != NULL){
					free(cmd[1]);
				}
				sleep(2);
				continue;
			}

			//create message
			//TCP MESSAGE FORMAT:  "messageLength:commandNumber:account,deposit/withdraw amount"
			strcpy(messsage, "3:");

			//delimiter betweeen account and deposit/withdraw amount is a comma ','
			char tmp[1041];
			strcpy(tmp, currentAcct);
			strcat(tmp, ",");
			strcat(tmp, cmd[1]);

			char len[6];
			sprintf(len, "%d", strlen(tmp));
			strcat(message, len);
			strcat(message, tmp);
			int ret = send(clientSocket, message, 1050, 0);

			if (ret == -1){
				printf("Failed to send message to server(3)\n");
				continue;
			}

			//wait for ACK in case deposit is unsuccessful
			strcpy(buffer, "");
			printf("Waiting for ACK\n");
			while(strcmp(buffer, "") == 0){
				read(clientSocket, buffer, 1024);
			}
			printf("ACK received\n");

			if (strcmp(buffer, "0") != 0){
				printf("Deposited %s into account %s successfully\n", cmd[1], currentAcct);
			}else{
				printf("ERROR: Could not deposit %s into account %s\n", cmd[1], currentAcct);
			}

		}else if(strcmp(cmd[0], "withdraw") == 0 && cmd[1] != NULL){
			printf("In withdraw\n");

			if (currentAcct == NULL){
				printf("User is not yet logged in.\n");
				free(cmd[0]);
				if (cmd[1] != NULL){
					free(cmd[1]);
				}
				sleep(2);
				continue;
			}

			//create message
			//TCP MESSAGE FORMAT:  "messageLength:commandNumber:account,deposit/withdraw amount"
			strcat(tail, "4:");

			char tmp[1041];
			strcpy(tmp, currentAcct);
			strcat(tmp, ",");
			strcat(tmp, cmd[1]);

			char len[6];
			sprintf(len, "%d", strlen(tmp));
			strcat(message, len);
			strcat(message, tmp);
			int ret = send(clientSocket, message, 1050, 0);

			if (ret == -1){
				printf("Failed to send message to server (4)\n");
				continue;
			}

			//wait for ACK in case deposit is unsuccessful
			strcpy(buffer, "");
			printf("Waiting for ACK\n");
			while(strcmp(buffer, "") == 0){
				read(clientSocket, buffer, 1024);
			}
			printf("ACK received\n");

			if (strcmp(buffer, "0") != 0){
				printf("Withdrew %s from account %s successfully\n", cmd[1], currentAcct);
			}else{
				printf("ERROR: Could not withdraw %s from account %s\n", cmd[1], currentAcct);
			}
			
			
		}else if(strcmp(cmd[0], "query") == 0){
			printf("In query\n");

			if (currentAcct == NULL){
				printf("User is not yet logged in\n");
				free(cmd[0]);
				if (cmd[1] != NULL){
					free(cmd[1]);
				}
				sleep(2);
				continue;
			}

			strcat(tail, ":5:");
			strcat(tail, currentAcct);

			char len[6];
			sprintf(len, "%d", strlen(tail));
			strcat(message, len);
			strcat(message, tail);
			int ret = send(clientSocket, message, 1050, 0);

			if (ret == -1){
				printf("Could not send to server (5)\n");
				continue;
			}

			//wait for ACK from server
			strcpy(buffer, "");
			printf("Waiting for ACK\n");
			while(strcmp(buffer, "") == 0){
				read(clientSocket, buffer, 1024);
			}
			printf("ACK received\n");

			if (strcmp(buffer, "0") != 0){
				char inputs[2];
				inputs[0] = NULL;
				inputs[1] = NULL;
				int length = 0;
				char* token;
				pos = 0;
				char* ptr = buffer;
				while((token = strtok_r(ptr, ":", &ptr))){
					inputs[pos] = (char*) malloc(sizeof(char)*((strlen(token))+1));
					strcpy(inputs[pos], token);
				}

				//check integrity of message received.
				length = atoi(inputs[0]);

				if (inputs[1] == NULL){
					printf("Error in message received format. Try again.\n");
					free(inputs[0]);
				}else if (length != strlen(inputs[1])){
					printf("Message received is corrupted. Balance received: %s. Try again.\n", inputs[1]);
					free(inputs[0]);
					free(inputs[1]);
				}else{
					printf("Account: %s\tBalance: %s\n", currentAct, inputs[1]);
					free(inputs[0]);
					free(inputs[1]);
				}

			}else{
				printf("ERROR: Could not get balance of account %s\n", currentAcct);
			}

			
		}else if(strcmp(cmd[0], "end") == 0){
			printf("In end\n");

			if (currentAcct == NULL){
				printf("User is not yet logged in\n");
			}else{
				printf("Logging out %s", currentAcct);
				free(currentAcct);
				currentAcct = NULL;
			}
			
		}else if(strcmp(cmd[0], "quit") == 0){
			printf("Quitting...\n");

			pthread_mutex_lock(mutex1);
			status = 0;
			pthread_mutex_unlock(mutex1);

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
	
	if (currentAcct != NULL){
		free(currentAcct);
	}

	close(clientSocket);
	pthread_join(thread);
	
	return 0;
}