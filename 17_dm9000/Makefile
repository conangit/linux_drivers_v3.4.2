KDIR := /home/hong/jz2440/kernel/linux-3.4.2

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=arm-linux-

.PHONY:
clean:
	make -C $(KDIR) M=$(PWD) modules clean ARCH=arm CROSS_COMPILE=arm-linux-
	rm -rf modules.order *.symvers

# obj-m += virtual_net.o
obj-m += dm9000_ext.o

