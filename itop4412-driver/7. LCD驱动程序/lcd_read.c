//标准输入输出头文件
#include <stdio.h>

//文件操作函数头文件
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>      

int main(int argc, char **argv)
{
	int fdfb;
	char data[400];			
	int i = 0;
	fdfb = open("/dev/fb0", O_RDWR);
	
	if(fdfb < 0){
		printf("error!\n");
		return -1;
	}
	
	read(fdfb, data, 400);
	for(i=0;i<400;i++)
	{	
		printf("%d",data[i]);
		if(i%20==0 && i!=0){
			printf("\n");
		}
	}
	close(fdfb);

	return 0;
}