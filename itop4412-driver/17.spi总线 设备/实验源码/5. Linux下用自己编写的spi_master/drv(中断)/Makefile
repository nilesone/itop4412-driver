obj-m += spi_oled_drv.o 
obj-m += spi_exynos4412.o

KDIR := /home/test/iTop4412_Kernel_3.0

PWD ?= $(shell pwd)


all:
	make -C $(KDIR) M=$(PWD) modules
		
clean:
	rm -rf *.o

	
	
	
