#!/bin/sh
if [ $ACTION = "add" ];
then
	mount /dev/sda1 /mnt/disk;
else
	umount /mnt/disk;
fi	