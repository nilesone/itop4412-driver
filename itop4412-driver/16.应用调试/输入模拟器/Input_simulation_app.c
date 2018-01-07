#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define INPUT_SIMULATION_START 0
#define INPUT_SIMULATION_CLEAN 1

/*
 *用法：Input_simulation_app write <file>
 *	   Input_simulation_app recurrent
 */

void print_usage(char *file)
{
	printf("Usage :\n");	
	printf("%s write <file>\n", file);	
	printf("%s recurrent\n", file);
}

int main(int argc, char **argv)
{
	int fd, mymsg_fd;
	int ret;
	char buff[128];

	if (argc != 2 && argc != 3)
		print_usage(argv[0]);

	fd = open("/dev/input_simulation", O_RDWR);
	if (fd < 0)
	{
		printf("open /dev/Input_simulation fail!\n");
		return -1;
	}

	if (strcmp(argv[1], "write") == 0)					//写入复现数据
	{
		mymsg_fd = open("/recurrent.txt", O_RDONLY);
		if (mymsg_fd < 0)
		{
			printf("open recurrent.txt fail!\n");
			return -1;
		}

		while(1)									
		{
			ret =read(mymsg_fd, buff, 128);			
			if (ret == 0)
			{
				printf("write ok!\n");
				ioctl(fd, INPUT_SIMULATION_CLEAN, 0);
				
				break;
			}
			
			ret = write(fd, buff, ret);
			if (ret < 0)
			{
				printf("write fail!\n");
				break;
			}
		}

		close(mymsg_fd);

	}
	else if (strcmp(argv[1], "recurrent") == 0)		//开始复现操作
	{
		printf("recurrent start\n");
		ioctl(fd, INPUT_SIMULATION_START, 0);
	}
	else
	{
		print_usage(argv[0]);
	}
	
	close(fd);
	
	return 0;
}

