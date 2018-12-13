#include "banking.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

char* currentAcct = NULL; //current account logged into. if NULL, no one is logged in.
int clientSocket;
int status;  //1: active   0: inactive

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; //status
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER; //currentAcct


char* trim(char* string){
	//trim column values
	//printf("in trim\n");
	if (strlen(string) == 0 || string == NULL){
		char* tmp = (char*) malloc(sizeof(char));
		tmp = "";
		return tmp;
	}
	int bpos = 0;
	int epos = strlen(string)-1;
	
	while(string[bpos] == ' '){
		bpos++;
	}
	
	while(string[epos] == ' ' || string[epos] == '\n'){
		epos--;
	}
	
	if (epos < bpos){
		char* tmp = (char*) malloc(sizeof(char));
		tmp = "";
		return tmp;
	}
	
	char* res = (char*)malloc(sizeof(char)*((epos-bpos)+2));
	int x, y;
	for (x = bpos, y = 0; x <= epos; x++, y++){
		res[y] = string[x];
	}
	res[y] = '\0';
	
	return res;
}

//Read messages from server and send them to user
void* responseOutput(void* arg){
	//This will be a different thread constantly listening to the server
	//char* buffer = (char*) malloc(sizeof(char)*1024);
	char* buffer;
	int valread;
	char** tokens = (char**) malloc(sizeof(char*)*3);
	
	while(1){
		pthread_mutex_lock(&mutex1);
		if (status == 0){
			pthread_mutex_unlock(&mutex1);
			break;
		}
		pthread_mutex_unlock(&mutex1);

		buffer = (char*) malloc(sizeof(char)*1024);
		tokens[0] = NULL;
		tokens[1] = NULL;
		tokens[2] = NULL;

    	valread = recv( clientSocket , buffer, 1024, 0);
    	//valread = read(clientSocket, buffer, 1024);
    	if (valread == -1 || strcmp(buffer, "x") == 0 || strcmp(buffer, "") == 0){
    		free(buffer);
	    	continue;
    	}
    	printf("ACK received.\n");


    	if(strcmp(buffer, "terminate") == 0){
    		printf("Server session terminated.\n");
			free(buffer);
    		pthread_mutex_lock(&mutex1);
    		pthread_mutex_lock(&mutex2);
    		status = 0;
    		if (currentAcct != NULL){
	    		free(currentAcct);
    		}
    		pthread_mutex_unlock(&mutex2);
    		pthread_mutex_unlock(&mutex1);
		//	pthread_exit(NULL);
			close(clientSocket);
			exit(0);
		}

		char* token;
		char* ptr = buffer;
		int pos = 0;

		while((token = strtok_r(ptr, ":", &ptr))){
			tokens[pos] = (char*) malloc(sizeof(char)*((strlen(token))+1));
			strcpy(tokens[pos], token);
			pos++;
		}

		//Process the message.
		//printf("Processing ACK...\n");
		processReceipt(tokens);
		//printf("Processing Complete\n");


		if (tokens[0] != NULL){
			free(tokens[0]);
		}
		if (tokens[1] != NULL){
			free(tokens[1]);
		}
		if (tokens[2] != NULL){
			free(tokens[2]);
		}
		
		free(buffer);
		buffer = NULL;
    }
	
	 free(tokens);
	 
    pthread_exit(NULL);
}


int main (int argc, char** argv){
	
	if (argc != 3){
		printf("Incorrect input format. Correct format: ./client [machine name] [port number]\n");
		exit(0);
	}
	
	char* machine = argv[1];
	char ip[100];
	struct hostent *he;
	int i;
	if((he=gethostbyname(argv[1]))==NULL)
	  {
	    herror("getname");
	  }

	strcpy(ip,inet_ntoa(*(struct in_addr *)he->h_addr));
	int port = atoi(argv[2]);

	struct hostent* host = gethostbyname(machine);
	
	if (host == NULL){
		printf("Could not get address of machine.\n");
		exit(0);
	}
	
	clientSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (clientSocket < 0){
		printf("Error in creating socket.\n");
		exit(0);
	}
	
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	memcpy(&addr.sin_addr, host->h_addr, host->h_length);

	if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0){
		printf("Invalid address/Address not supported\n");
		exit(0);	
	}
	
	printf("Attempting to connect...\n");
	while(connect(clientSocket, (struct sockaddr*) &addr, sizeof(addr)) < 0){
		printf("Retrying...\n");
		sleep(3);
	}
	printf("Connection successful.\n");

	status = 1;
	
	pthread_t thread;
	pthread_create(&thread, NULL, responseOutput, NULL);
	
	
	char* buffer;
	char** cmd = (char**) malloc(sizeof(char*)*2);
	
	while(1){
		pthread_mutex_lock(&mutex1);
		if (status == 0){
			break;
		}
		pthread_mutex_unlock(&mutex1);

		//char* cmd[2];
		buffer = (char*) malloc(sizeof(char)*1024);
		cmd[0] = NULL;
		cmd[1] = NULL;
		
		char* token;
		char* ptr;
		int pos = 0;
		int ret = 0;
		
		printf("Enter command:\n");
		fgets(buffer, 255, stdin);
		
		ptr = buffer;
		while((token = strtok_r(ptr, " ", &ptr))){
			cmd[pos] = trim(token);
			pos++;
		}
		//printf("cmd[0]: %s\tcmd[1]: %s\n", cmd[0], cmd[1]);
		
		//  *NOTE*  TCP message format (create, serve):   "commandNumber:messageLength:message"
		//  *NOTE*  TCP message format(deposit, withdraw): "commandNumber:messageLength:deposit/withdraw amount"
		//  ACK:   0: error   else ==> success

		processInputs(cmd);
		
		if (cmd[0] != NULL){
			free(cmd[0]);
		}
		if (cmd[1] != NULL){
			free(cmd[1]);
		}
		
		free(buffer);
		sleep(2);
	}
	
	free(cmd);
	pthread_join(thread, NULL);
	printf("Disconnected from server\n");
	
	if (currentAcct != NULL){
		free(currentAcct);
	}

	close(clientSocket);
	
	return 0;
}

//Used to process client input commands
void processInputs(char** cmd){
	char message[1050] = "";
	int ret = 0;

	if (strcmp(cmd[0], "create") == 0 && cmd[1] != NULL){ 
		//printf("In create\n");

		//Check if user is already logged in
		pthread_mutex_lock(&mutex2);
		if (currentAcct != NULL){
			printf("User %s already logged in. Must be logged out to create\n", currentAcct);
			pthread_mutex_unlock(&mutex2);
			return;
		}
		pthread_mutex_unlock(&mutex2);


		//create message
		//TCP MESSAGE FORMAT:  "commandNumber:messageLength:message"
		strcpy(message, "1:");

		char len[6];			//variable to hold length of message (account)
		sprintf(len, "%d", strlen(cmd[1]));   //convert length of message into string

		strcat(message, len);
		strcat(message, ":");
		strcat(message, cmd[1]);
		//printf("Message to be sent: %s\n", message);
		ret = send(clientSocket, message, 1050, 0);   //might have to cast message to void*
			
	}else if(strcmp(cmd[0], "serve") == 0 && cmd[1] != NULL){
		//printf("In serve\n");

		//Check if user already logged in.
		pthread_mutex_lock(&mutex2);
		if (currentAcct != NULL){
			printf("User %s already logged in. Must be logged out to serve\n", currentAcct);
			pthread_mutex_unlock(&mutex2);
			return;
		}
		pthread_mutex_unlock(&mutex2);
			
		//create message
		//TCP MESSAGE FORMAT:  "commandNumber:messageLength:message"
		strcpy(message, "2:");

		char len[6];			//variable to hold length of message (account)
		sprintf(len, "%d", strlen(cmd[1]));   //convert length of message into string

		strcat(message, len);
		strcat(message, ":");
		strcat(message, cmd[1]);
		//printf("Message to be sent: %s\n", message);
		ret = send(clientSocket, message, 1050, 0);   //might have to cast message to void*
			
	}else if(strcmp(cmd[0], "deposit") == 0 && cmd[1] != NULL){
		//printf("In deposit\n");

		char tmp[1041];
		pthread_mutex_lock(&mutex2);
		if (currentAcct == NULL){
			pthread_mutex_unlock(&mutex2);
			printf("User is not yet logged in.\n");
			return;
		}
		strcpy(tmp, currentAcct);
		pthread_mutex_unlock(&mutex2);

		//create message
		//TCP MESSAGE FORMAT:  "commandNumber:messageLength:deposit/withdraw amount"
		strcpy(message, "3:");

		//delimiter betweeen account and deposit/withdraw amount is a comma ','
		//strcat(tmp, ",");
		//strcat(tmp, cmd[1]);

		char len[6];
		//sprintf(len, "%d", strlen(tmp));
		sprintf(len, "%d", strlen(cmd[1]));
		strcat(message, len);
		strcat(message, ":");
		//strcat(message, tmp);
		strcat(message, cmd[1]);
		//printf("Message to be sent: %s\n", message);
		ret = send(clientSocket, message, 1050, 0);     //might have to cast message to void*

	}else if(strcmp(cmd[0], "withdraw") == 0 && cmd[1] != NULL){
		//printf("In withdraw\n");

		char tmp[1041];
		pthread_mutex_lock(&mutex2);
		if (currentAcct == NULL){
			pthread_mutex_unlock(&mutex2);
			printf("User is not yet logged in.\n");
			return;
		}
		strcpy(tmp, currentAcct);
		pthread_mutex_unlock(&mutex2);

		//create message
		//TCP MESSAGE FORMAT:  "commandNumber:messageLength:deposit/withdraw amount"
		strcat(message, "4:");

		//strcat(tmp, ",");
		//strcat(tmp, cmd[1]);

		char len[6];
		//sprintf(len, "%d", strlen(tmp));
		sprintf(len, "%d", strlen(cmd[1]));
		strcat(message, len);
		strcat(message, ":");
		//strcat(message, tmp);
		strcat(message, cmd[1]);
		//printf("Message to be sent: %s\n", message);
		ret = send(clientSocket, message, 1050, 0);    //might have to cast message to void*	
			
	}else if(strcmp(cmd[0], "query") == 0){
		//printf("In query\n");

		char tmp[1041];
		pthread_mutex_lock(&mutex2);
		if (currentAcct == NULL){
			pthread_mutex_unlock(&mutex2);
			printf("User is not yet logged in\n");
			return;
		}
		strcpy(tmp, currentAcct);
		pthread_mutex_unlock(&mutex2);

		strcpy(message, "5:");

		char len[6];
		sprintf(len, "%d", strlen(tmp));
		strcat(message, len);
		strcat(message, ":");
		strcat(message, tmp);
		//printf("Message to be sent: %s\n", message);
		ret = send(clientSocket, message, 1050, 0);
			
	}else if(strcmp(cmd[0], "end") == 0){
		//printf("In end\n");
		
		char tmp[1041];
		pthread_mutex_unlock(&mutex2);
		if (currentAcct == NULL){
			pthread_mutex_unlock(&mutex2);
			printf("User is not yet logged in\n");
			return;
		}
		
		strcpy(tmp, currentAcct);
		pthread_mutex_unlock(&mutex2);
		
		strcpy(message, "6:");
		char len[6];
		sprintf(len, "%d", strlen(tmp));
		strcat(message, len);
		strcat(message, ":");
		strcat(message, tmp);
		//printf("Message to be sent: %s\n", message);
		ret = send(clientSocket, message, 1050, 0);		
			
	}else if(strcmp(cmd[0], "quit") == 0){
		printf("Quitting...\n");

		strcpy(message, "7:4:");
		//Don't need to worry about receiving ACK from server.
		pthread_mutex_lock(&mutex1);
		pthread_mutex_lock(&mutex2);
		status = 0;
		if (currentAcct != NULL){
			strcat(message, currentAcct);
		}else{
			strcat(message, "quit");
			pthread_mutex_unlock(&mutex2);
			pthread_mutex_unlock(&mutex1);
		}
		pthread_mutex_unlock(&mutex2);
		pthread_mutex_unlock(&mutex1);

		printf("Message sent successfully.\nWaiting for ACK...\n");
		ret = send(clientSocket, message, 1050, 0);
		printf("ACK received.\nQuit client session successfully.\n");
		return;

	}else{
		printf("Invalid command. Try again\n");
		return;
	}

	if (ret == -1 || ret == 0){
		printf("Failed to send message to server\n");
	}else{
		printf("Message sent successfully.\nWaiting for ACK...\n");
	}

	return;
}


//Used to process the message received from the server
void processReceipt(char** tokens){
  
	if (tokens[0] == NULL || tokens[1] == NULL || tokens[2] == NULL){
		printf("Incorrect message format\n");
		return;
	}

	int length = atoi(tokens[1]);
	//printf("length: %d\n", length);
	if (length != strlen(tokens[2])){
		printf("ERROR: Message received is corrupted. Message: %s\nMessage length should be: %s\n", tokens[2], tokens[1]);
		return;
	}

	if (strcmp(tokens[0], "0") == 0){
		printf("ERROR: %s\n", tokens[2]);
		return;
	}

	char tmpAcct[256];
	pthread_mutex_lock(&mutex2);
	if (currentAcct != NULL){
		strcpy(tmpAcct, currentAcct);
	}
	pthread_mutex_unlock(&mutex2);


	if (strcmp(tokens[0], "1") == 0){				//ACK for "create"
		printf("Account: %s  created\n", tokens[2]);

	}else if (strcmp(tokens[0], "2") == 0){			//ACK for "server"
		pthread_mutex_lock(&mutex2);
		currentAcct = (char*) malloc(sizeof(char)*(strlen(tokens[2])+1));
		strcpy(currentAcct, tokens[2]);
		pthread_mutex_unlock(&mutex2);
		printf("Logged into account: %s\n", tokens[2]);

	}else if (strcmp(tokens[0], "3") == 0){			//ACK for "deposit"
		printf("Deposited $%s into account %s\n", tokens[2], tmpAcct);

	}else if (strcmp(tokens[0], "4") == 0){			//ACK for 'withdraw'
		printf("Withdrew $%s from account %s\n", tokens[2], tmpAcct);

	}else if (strcmp(tokens[0], "5") == 0){			//ACK for "query"
		printf("Account: %s\tBalance: %s\n", tmpAcct, tokens[2]);

	}else if (strcmp(tokens[0], "6") == 0){			//ACK for "end"
		pthread_mutex_lock(&mutex2);
		printf("Logging out %s\n", currentAcct);
		free(currentAcct);
		currentAcct = NULL;
		pthread_mutex_unlock(&mutex2);

	}else if (strcmp(tokens[0], "7") == 0){
		printf("Quit client session successfully\n");

	}else{
		printf("Received invalid message: %s:%s:%s\n", tokens[0], tokens[1], tokens[2]);
	}

	return;
}
