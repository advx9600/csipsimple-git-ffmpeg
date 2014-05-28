#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h> /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "my_serial.h"

static int gsd=0;

int create_my_serial(const char* dev)
{
	int sd = open(dev,O_RDWR);
	if (sd <0) perror("open");
	gsd = sd;
	return gsd;
}

void close_my_serial()
{
	if (gsd>0) close(gsd);
}

int read_my_serial(char* buf,int len)
{
	int readlen = read(gsd,buf,len);
	if (readlen <0) perror("read");
	return readlen;
}

int poll_my_serial(int polltime)
{
	struct pollfd pollfds;
        pollfds.fd = gsd;
        pollfds.events = POLLIN;
	return poll(&pollfds,1,polltime);
}

int write_my_serial(char* buf,int len)
{
	int writelen=write(gsd,buf,len);
	if (writelen <0) perror("write");
	return writelen;
}


#if 0

int main()
{
	char buf[1024*10];
	int readlen=0;
	int count=0;

	create_my_serial("/dev/ttySAC0");

	count=0;
	while(1)
	{
		readlen=read_my_serial(buf,sizeof(buf));
		buf[readlen]='\0';
		printf("data:%s\n",buf);
		continue;

		if (poll_my_serial(1000)){
		count++;
		read_my_serial(buf,sizeof(buf));
		printf("get data %d\n",count);
		read_my_serial(buf,sizeof(buf));
		}else{
		printf("time out\n");
		}
	}

	close_my_serial();
	return 0;
}
#endif
