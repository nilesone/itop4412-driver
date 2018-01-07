
测试：
1. insmod ramblock.ko
2. 格式化：mkdosfs /dev/ramblock_disk
3. 挂接：mount /dev/ramblock_disk /mnt/disk2
4. 读写文件：cd /mnt/disk2, vi hello.c
5. 保存镜像文件：cat /dev/ramblock_disk > ramblock.bin
6. 在pc上查看ramblock.bin
	mount -o loop ramblock.bin /mnt						//-o loop可以把普通文件当做块设备挂接
	ls /mnt

	
知识点：电梯调度算法
	1. 对磁盘的操作是read是一起的，write也是一起的
	2. 对磁盘的写操作不会立即执行，但可以用sync命令来让其立即执行