//标准输入输出头文件
#include <stdio.h>

//文件操作函数头文件
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define LINE_SIZE 800*3           //一行像素所占的字节数

int main(int argc, char **argv)
{
	int fdf, fdfb, fdn, ret, i;
	char file_head[54];					//bmp文件头部信息
	char color_data[4];					//一个像素的数据
	char line_data[LINE_SIZE];			//一行像素的数据
	
	fdf = open(argv[1],O_RDWR);
	
	fdfb = open("/dev/fb0", O_RDWR);
	
	if(fdf < 0 || fdfb < 0){
		printf("error!\n");
		return -1;
	}
	
	read(fdf, file_head, 54);
	
	while(1)
	{
		
		ret = read(fdf, line_data, LINE_SIZE);		//从源文件读出数据
		if(ret <= 0)	break;
		
		for(i=0;i<800;i++){						//人工实现左右翻转
			
			color_data[0] = line_data[LINE_SIZE - i*3 -2 -1];
			color_data[1] = line_data[LINE_SIZE - i*3 -1 - 1];
			color_data[2] = line_data[LINE_SIZE - i*3 -0 - 1];
			color_data[3] = 0;
			write(fdfb, color_data, 4);				//写入目标文件
		}
		
	}
	close(fdfb);
	close(fdf);
	return 0;
}