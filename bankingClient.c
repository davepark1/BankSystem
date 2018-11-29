#include "banking.h"


char* currentAcct = NULL; //current account logged into. if NULL, no one is logged in.

//Read messages from server and send them to user
void* responseOutput(void* arg){
	//This will be a different thread constantly listening to the server
}



int main (int argc, char** argv){
	if (argc != 3){
		printf("Incorrect input format. Correct format: ./bankingClient [machine name] [port number]\n");
		exit(0);
	}
	
	char* machine = argv[1];
	int port = atoi(argv[2]);
	
	pthread_t thread;
	pthread_create(&thread, NULL, responseOutput, NULL);
	
	
}