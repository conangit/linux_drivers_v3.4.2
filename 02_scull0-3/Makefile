KDIR := /home/hong/jz2440/kernel/linux-3.4.2

CR := arm-linux-

all:
	make -C $(KDIR) M=$(PWD) modules ARCH=arm CROSS_COMPILE=$(CR)

.PHONY:
clean:
	make -C $(KDIR) M=$(PWD) modules clean ARCH=arm CROSS_COMPILE=$(CR)
	rm -rf modules.order *.symvers

obj-m += scull.o container_of.o
