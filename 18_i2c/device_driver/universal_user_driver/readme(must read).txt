自定义struct i2c_msg和struct i2c_rdwr_ioctl_data结构
和包含头文件#include <linux/i2c.h> #include <linux/i2c-dev.h>
表现如此不同的原因在于----叉编译器的头文件之间的差异，如下：

gcc-3.4.5
#find -name i2c.h
./arm-linux/include/linux/i2c.h
./arm-linux/sys-include/linux/i2c.h
#find -name i2c-dev.h
./arm-linux/include/linux/i2c-dev.h
./arm-linux/sys-include/linux/i2c-dev.h


gcc-4.3.2
#find -name i2c.h
./arm-none-linux-gnueabi/libc/usr/include/linux/i2c.h
#find -name i2c-dev.h
./arm-none-linux-gnueabi/libc/usr/include/linux/i2c-dev.h

gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf
#find -name i2c.h
./arm-linux-gnueabihf/libc/usr/include/linux/i2c.h
#find -name i2c-dev.h
./arm-linux-gnueabihf/libc/usr/include/linux/i2c-dev.h


gcc-3.4.5
在文件./arm-linux/include/linux/i2c.h和文件./arm-linux/sys-include/linux/i2c.h中
struct i2c_msg {
    __u16 addr;                     /* slave address            */
    __u16 flags;        
#define I2C_M_TEN           0x10    /* we have a ten bit chip address   */
#define I2C_M_RD            0x01
#define I2C_M_NOSTART       0x4000
#define I2C_M_REV_DIR_ADDR  0x2000
#define I2C_M_IGNORE_NAK    0x1000
#define I2C_M_NO_RD_ACK     0x0800
    __u16 len;                      /* msg length               */
    __u8 *buf;                      /* pointer to msg data          */
};
而在文件中./arm-linux/include/linux/i2c-dev.h和./arm-linux/sys-include/linux/i2c-dev.h中
struct i2c_msg {
    __u16 addr;                     /* slave address            */
    unsigned short flags;       
#define I2C_M_TEN   0x10            /* we have a ten bit chip address   */
#define I2C_M_RD    0x01
#define I2C_M_NOSTART   0x4000
#define I2C_M_REV_DIR_ADDR  0x2000
#define I2C_M_IGNORE_NAK    0x1000
#define I2C_M_NO_RD_ACK     0x0800
    short len;                      /* msg length               */
    char *buf;                      /* pointer to msg data          */
    int err;
    short done;
};

从而造成头文件#include <linux/i2c.h> #include <linux/i2c-dev.h>不能同时包含，且定义又不同，造成了软件上变现上的差异
而在4.3.2版本编译器中，则不存在此问题，因为头文件的定义严格跟内核保持一致，即：
在<linux/i2c.h>中定义struct i2c_msg
在include <linux/i2c-dev.h>中定义struct i2c_rdwr_ioctl_data



