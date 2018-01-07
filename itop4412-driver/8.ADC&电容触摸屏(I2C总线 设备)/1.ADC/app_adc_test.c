#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



int main(void)
{
	int fd, ret, value;
	
	fd = open("/dev/myadc", O_RDWR);
	
	if(fd<0){
		printf("Open /dev/myadc fail!\n");
		return -1;
	}
	
	read(fd, &value, sizeof(value));
	
	printf("Adc's value is %d\n", value);
	
	close(fd);
	return 0;
}