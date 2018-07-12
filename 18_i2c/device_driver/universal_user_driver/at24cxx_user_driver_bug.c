#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>


#if 0
//直接导入头文件模式下 在2.6.22.6内核版本中 不支持多条消息
//此文件为多条消息拆开使用ioctl
// #include <linux/i2c.h>
#include <linux/i2c-dev.h>
#else

//直接自定义数据结构模式下 在2.6.22.6内核版本中 支持多条消息
#define I2C_RDWR    0x0707
#define I2C_M_RD    0x0001

struct i2c_msg {
    unsigned short addr;    /* slave address            */
    unsigned short flags;
    unsigned short len;     /* msg length               */
    unsigned char *buf;     /* pointer to msg data      */
};

/* This is the structure as used in the I2C_RDWR ioctl call */
struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;   /* pointers to i2c_msgs */
    unsigned int nmsgs;     /* number of i2c_msgs */
};
#endif

//适配AT24C128_256_512 EEPROM
#define LH_AT24C256_ADDR    0x50
#define LH_WRITE_ADDR       0x1234
#define LH_WRITE_DATA       0x5d

static int i2c_test(void)
{
    int fd;
    struct i2c_rdwr_ioctl_data eeprom_data;
    int ret;
    
    //1.打开通用设备文件
    fd = open("/dev/i2c-0", O_RDWR);
    if (fd < 0)
    {
        printf("Open device i2c-0 failed\n");
        return -1;
    }
    printf("Open device i2c-0...\n");

    //写-1条消息 读-2条消息
    eeprom_data.msgs = (struct i2c_msg *)malloc(sizeof(struct i2c_msg) * 2);
    if (NULL == eeprom_data.msgs)
    {
        printf("No memory for msg\n");
        return -1;
    }

    //2.构造写数据到eeprom的消息
    eeprom_data.nmsgs = 1;
    (eeprom_data.msgs[0]).addr   = LH_AT24C256_ADDR;
    (eeprom_data.msgs[0]).flags  = 0;             //写
    (eeprom_data.msgs[0]).len    = 3;
    (eeprom_data.msgs[0]).buf = (unsigned char *)malloc(3);
    if (NULL == (eeprom_data.msgs[0]).buf)
    {
        printf("No memory for msg write buf\n");
        free(eeprom_data.msgs);
        return -1;
    }
    (eeprom_data.msgs[0]).buf[0] = (unsigned char)(LH_WRITE_ADDR >> 8);          //addr_H
    (eeprom_data.msgs[0]).buf[1] = (unsigned char)(LH_WRITE_ADDR & 0xff);        //addr_L
    (eeprom_data.msgs[0]).buf[2] = LH_WRITE_DATA;                                //data

    //3.使用ioctl写入数据
    ret = ioctl(fd, I2C_RDWR, (unsigned long)&eeprom_data);
    if (ret < 0)
    {
        printf("I2C write failed: %d\n", ret);
        free((eeprom_data.msgs[0]).buf);
        free(eeprom_data.msgs);
        return ret;
    }

    //必须存在！！！！
    //sleep(1);
    usleep(1);

    //4.构造从eeprom读数据的消息
    //写过程
    eeprom_data.nmsgs = 1;
    (eeprom_data.msgs[0]).addr   = LH_AT24C256_ADDR;
    (eeprom_data.msgs[0]).flags  = 0;             //写
    (eeprom_data.msgs[0]).len    = 2;
    (eeprom_data.msgs[0]).buf[0] = (unsigned char)(LH_WRITE_ADDR >> 8);          //addr_H
    (eeprom_data.msgs[0]).buf[1] = (unsigned char)(LH_WRITE_ADDR & 0xff);        //addr_L

    ret = ioctl(fd, I2C_RDWR, (unsigned long)&eeprom_data);
    if (ret < 0)
    {
        printf("I2C write and read failed: %d\n", ret);
        free((eeprom_data.msgs[0]).buf);
        free(eeprom_data.msgs);
        return ret;
    }

    //读过程
    eeprom_data.nmsgs = 1;
    (eeprom_data.msgs[0]).addr   = LH_AT24C256_ADDR;
    (eeprom_data.msgs[0]).flags  = I2C_M_RD; //I2C_M_RD;             //读
    (eeprom_data.msgs[0]).len    = 1;
    (eeprom_data.msgs[0]).buf = (unsigned char *)malloc(1);
    if (NULL == (eeprom_data.msgs[0]).buf)
    {
        printf("No memory for msg read buf\n");
        free((eeprom_data.msgs[0]).buf);
        free(eeprom_data.msgs);
        return -1;
    }
    (eeprom_data.msgs[0]).buf[0] = 0;
    
    //5.使用ioctl读出数据
    ret = ioctl(fd, I2C_RDWR, (unsigned long)&eeprom_data);
    if (ret < 0)
    {
        printf("I2C read failed: %d\n", ret);
        free((eeprom_data.msgs[0]).buf);
        free(eeprom_data.msgs);
        return ret;
    }

    printf("write data: 0x%x\n", LH_WRITE_DATA);
    printf("read data: 0x%x\n", eeprom_data.msgs[0].buf[0]);

    //6.关闭设备
    free((eeprom_data.msgs[0]).buf);
    free(eeprom_data.msgs);
    close(fd);
}

int main(void)
{
    int i;

    for (i = 0; i < 1; i++)
    {
        printf("test cnt:[%d]\n", i);
        sleep(1);
        i2c_test();
    }

    return 0;
}


