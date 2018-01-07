#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

/*
 * 用法：读：     ./regeditor r8 addr [num]
 *      写：   ./regeditor w8 addr val
 */

#define ker_r8   3
#define ker_r16  4
#define ker_r32  5

#define ker_w8   6
#define ker_w16  7
#define ker_w32  8


void printf_usage(char *file)
{
	printf("Usage: ./%s r8 addr [num]\n", file);
	printf("Usage: ./%s w8 addr val\n", file);
}

int main(int argc, char **argv)
{
	int fd;
	unsigned int i, num;
	unsigned int buff[2];

	if(argc != 3 && argc != 4)
	{
		printf_usage(argv[0]);
		return -1;
	}

	fd = open("/dev/ker_rw", O_RDWR);
	if(fd < 0)
	{
		printf("open /dev/ker_rw fail!\n");
		return -2;
	}

	buff[0] = strtoul(argv[2], NULL, 0);
	
	if(strcmp(argv[1], "r8") == 0)
	{
		if(argc == 3)
		{
			ioctl(fd, ker_r8, buff);
			printf("%s = 0x%x\n", argv[2], (unsigned char)buff[1]);
		}
		else if(argc == 4)
		{
			num = strtoul(argv[3], NULL, 0);
			for(i=0; i<num; i++)
			{
				ioctl(fd, ker_r8, buff);
				printf("%s + 0x%x = 0x%x\n", argv[2], i, (unsigned char)buff[1]);
				buff[0] += 1;
			}
		}
	}
	else if(strcmp(argv[1], "r16") == 0)
	{
		if(argc == 3)
		{
			ioctl(fd, ker_r16, buff);
			printf("%s = 0x%x\n", argv[2], (unsigned short)buff[1]);
		}
		else if(argc == 4)
		{
			num = strtoul(argv[3], NULL, 0);
			for(i=0; i<num; i++)
			{
				ioctl(fd, ker_r16, buff);
				printf("%s + 0x%x = 0x%x\n", argv[2], i*2, (unsigned short)buff[1]);
				buff[0] += 2;
			}
		}
	}
	else if(strcmp(argv[1], "r32") == 0)
	{
		if(argc == 3)
		{
			ioctl(fd, ker_r32, (unsigned long)buff);
			printf("%s = 0x%x\n", argv[2], (unsigned int)buff[1]);
		}
		else if(argc == 4)
		{
			num = strtoul(argv[3], NULL, 0);
			for(i=0; i<num; i++)
			{
				ioctl(fd, ker_r32, buff);
				printf("%s + 0x%x = 0x%x\n", argv[2], i*4, (unsigned int)buff[1]);
				buff[0] += 4;
			}
		}
	}
	else if(strcmp(argv[1], "w8") == 0)
	{
		num     = strtoul(argv[3], NULL, 0);
		buff[1] = num;
		ioctl(fd, ker_w8, buff);
	}
	else if(strcmp(argv[1], "w16") == 0)
	{
		num     = strtoul(argv[3], NULL, 0);
		buff[1] = num;
		ioctl(fd, ker_w16, buff);
	}
	else if(strcmp(argv[1], "w32") == 0)
	{
		num     = strtoul(argv[3], NULL, 0);
		buff[1] = num;
		ioctl(fd, ker_w32, buff);
	}
	else
	{
		printf_usage(argv[0]);
		return -3;
	}

	close(fd);
	
	return 0;
}

