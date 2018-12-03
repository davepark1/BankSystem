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

struct Acct{
	char* name;   //need to dynamically allocate when struct is created.
	double balance = 0;//starting value will be 0
	int session;  // 1: true     0: false
};

void* responseOutput(void* arg);

#endif
