#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "filesave.h"

FILE* gFd;

int open_file(const char* file_name)
{
	FILE* fd = fopen(file_name,"wb");
	if (fd == NULL){
		perror("fpen");
		return -1;
	}
	gFd=fd;
	return 0;
}
 
void close_file()
{
	if (gFd!=NULL) fclose(gFd);
}

int write_file(const char* buf,int len)
{
	return fwrite(buf,1,len,gFd);
}

#if 0
int main()
{
}
#endif
