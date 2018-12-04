#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>

typedef struct account{
  char* name;
  double balance;
  int service;//1 if currently being service 0 if not
}account;

typedef struct userinfo{
	int newsocket;
	
}userinfo;


