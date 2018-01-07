#include <linux/input.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char * const argv[])
{
    int fd = 0;
    struct input_event event[2] = {0};      //3！！！，驱动上传了2个事件，第三个用来装空元素 
    int ret = 0;
    fd = open("/dev/input/event1",O_RDONLY);
    while(1){
        ret = read(fd,&event,sizeof(event));
        printf("code0:%d,value0:%d\n",event[0].code,event[0].value);          //2！！！，最后一个是空
		printf("code1:%d,value1:%d\n",event[1].code,event[1].value);
	}
    return 0;
}