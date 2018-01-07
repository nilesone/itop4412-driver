#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int fd;

void my_sianal(int signum){
	unsigned char buff[2];
	read(fd, buff, sizeof(buff));
	printf("button[0] is %d\n", buff[0]);
	printf("button[1] is %d\n", buff[1]);
}

int main(void)
{
	unsigned int Oflags;
	signal(SIGIO, my_sianal);
	fd = open("/dev/pollkey", O_RDWR);
	if(fd < 0)
	{
		printf("open error!\n");
		return -1;
	}
	fcntl(fd, F_SETOWN, getpid());
	Oflags = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, Oflags | FASYNC);
	while(1)
	{
		sleep(1000);
	}
	return 0;
}