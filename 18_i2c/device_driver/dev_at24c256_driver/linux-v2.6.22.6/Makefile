KDIR := /home/hong/jz2440/kernel/linux-2.6.22.6

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-

.PHONY:
clean:
	make -C $(KDIR) M=$(PWD) modules clean ARCH=arm CROSS_COMPILE=arm-linux-
	rm -rf modules.order *.symvers

obj-m += at24c256_drv_3.o

