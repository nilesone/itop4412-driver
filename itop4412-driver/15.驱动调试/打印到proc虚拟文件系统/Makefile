obj-m += myprintk_proc.o 
obj-m += drv_test.o 

KDIR := /home/test/iTop4412_Kernel_3.0

PWD ?= $(shell pwd)


all:
	make -C $(KDIR) M=$(PWD) modules
		
clean:
	rm -rf *.o

	
	
	
