KDIR := /home/hong/jz2440/kernel/linux-3.4.2

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-

.PHONY:
clean:
	make -C $(KDIR) M=$(PWD) modules clean ARCH=arm CROSS_COMPILE=arm-linux-
	rm -rf modules.order *.symvers

obj-m += at24c256_drv.o
obj-m += at24c256_dev.o

obj-m += driver_device.o


