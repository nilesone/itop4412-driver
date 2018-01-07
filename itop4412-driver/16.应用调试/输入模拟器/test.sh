insmod /mnt/disk/myprintk_proc.ko
insmod /mnt/disk/ts_dev.ko
insmod /mnt/disk/ts_drv.ko
./mnt/disk/Input_simulation_app write recurrent.txt
./opt/tslib1.4/bin/ts_test &
sleep 1
./mnt/disk/Input_simulation_app recurrent
