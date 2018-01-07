#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
	int fd;
	unsigned int flag;
	fd = open("/dev/myled", O_RDWR);
	if(fd<0)
	{
		printf("Open /dev/myled fail!\n");
		return -1;
	}
	if(strcmp("on", argv[1]) == 0)
	{
		printf("/dev/myled on!\n");
		flag = 1;
		write(fd, &flag, sizeof(flag));
	}
	else if(strcmp("off", argv[1]) == 0)
	{
		printf("/dev/myled off!\n");
		flag = 0;
		write(fd, &flag, sizeof(flag));
	}
	else
	{
		printf("Please input on or off!\n");
		return -1;
	}
	close(fd);
	return 0;
}
