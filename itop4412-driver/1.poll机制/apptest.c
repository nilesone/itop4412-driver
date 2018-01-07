#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>

int main(int argc, char **argv)
{
	int fd;
	char *pollkey = "/dev/pollkey";
	unsigned char buff[2];
	int ret,count = 0;
	struct pollfd fds[1];
	
	if((fd = open(pollkey, O_RDWR))<0)
	{
		printf("open fail!\n");
		return -1;
	}
	fds[0].fd = fd;
	fds[0].events = POLLIN;
	while(1){
		ret = poll(fds, 1, 5000);
		if(!ret){
			printf("time out!count is %d\n",++count);
		}
		else
		{
			read(fd, buff, sizeof(buff));
			printf("button[0] value is %d\n",buff[0]);
			printf("button[1] value is %d\n",buff[1]);
		}
	}
	close(fd);
	return 0;
}