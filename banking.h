#ifndef BANKING_H
#define BANKING_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <semaphore.h>


typedef struct Acct{//typdef would be better here PJ
	char* name;   //need to dynamically allocate when struct is created.
	double balance;
	int service;  // 1: true     0: false
}Acct;

typedef struct userinfo{
	int newsocket;
	
}userinfo;

char* trim(char* string);

void* responseOutput(void* arg);

void processInputs(char** cmd);

void processReceipt(char** tokens);


#endif
