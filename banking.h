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
#include <limits.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct Acct{//typdef would be better here PJ
	char* name;   //need to dynamically allocate when struct is created.
	double balance;
	int session;  // 1: true     0: false
}Acct;

void* responseOutput(void* arg);

#endif
