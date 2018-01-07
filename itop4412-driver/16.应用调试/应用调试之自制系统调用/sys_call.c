#include <stdio.h>
#include <unistd.h>

int hello_test(char *buff, int count)
{
	int ret;

	{								
		   register int _a1 asm ("r0"), _a2 asm ("r1"), _nr asm ("r7"); 	//_a1表示r0寄存器，_a2表示r1寄存器，_nr表示r7寄存器并存放系统调用号
		   _a1 = (int)buff;							//第一个参数
		   _a2 = count;							//第二个参数
		   _nr = 376;						
		   asm volatile ("swi	0x0 @ syscall "	//实际上是svc指令
				 : "=r" (_a1)					//返回值存放在_a1(r0)寄存器中
				 : "r" (_a1), "r" (_a2)			//传入的参数
				 : "memory");					//会改变的地址
		   ret = _a1; }						    //返回值保存到ret中

	
	return ret;
}

int main(int argc, char **argv)
{
	int ret;
	
	//	ret = syscall(376, "Hello,I am main!", 17);
	ret = hello_test("Hello,I am main!", 17);

	if(ret == 17)
	{
		printf("success!\n");
	}
	
	return 0;
}
