#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>

#if 1
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#else
#define I2C_TIMEOUT     0x0702
#define I2C_RETRIES     0x0701
#define I2C_RDWR        0x0707
#define I2C_M_RD        0x0001

struct i2c_msg {
    unsigned short addr;
    unsigned short flags;
    unsigned short len;
    unsigned char *buf;
};

struct i2c_rdwr_ioctl_data {
    struct i2c_msg *msgs;
    unsigned int nmsgs;
};
#endif


//适配AT24C128_256_512 EEPROM
#define AT24C256_ADDR 0x50

static int i2c_test(char *dev, unsigned short addr, unsigned char data)
{
    int fd;
    struct i2c_rdwr_ioctl_data eeprom_data;
    int ret;
    
    //1.打开通用设备文件
    fd = open(dev, O_RDWR);
    if (fd < 0)
    {
        printf("Open device %s failed\n", dev);
        return -1;
    }
    printf("Open device %s...\n", dev);

#if 1
    /*
     * 在i2c-dev层次 value in units of 10 ms
     * client->adapter->timeout = msecs_to_jiffies(arg * 10);
     * 在驱动层次 int timeout, in jiffies
     */
    /*
     * s3c2410 i2c未实现此参数
     */
    ret = ioctl(fd, I2C_TIMEOUT, 2);    //20ms
    if (ret < 0)
    {
        printf("I2C set I2C_TIMEOUT failed: %d\n", ret);
        return ret;
    }
#endif

#if 0
    ret = ioctl(fd, I2C_RETRIES, 2);
    if (ret < 0)
    {
        printf("I2C set I2C_RETRIES failed: %d\n", ret);
        return ret;
    }
#endif

    //写-1条消息 读-2条消息
    eeprom_data.msgs = (struct i2c_msg *)malloc(sizeof(struct i2c_msg) * 2);
    if (NULL == eeprom_data.msgs)
    {
        printf("No memory for msg\n");
        return -1;
    }

    //2.构造写数据到eeprom的消息
    eeprom_data.nmsgs = 1;
    (eeprom_data.msgs[0]).addr   = AT24C256_ADDR;
    (eeprom_data.msgs[0]).flags  = 0;             //写
    (eeprom_data.msgs[0]).len    = 3;
    (eeprom_data.msgs[0]).buf = (unsigned char *)malloc(3);
    if (NULL == (eeprom_data.msgs[0]).buf)
    {
        printf("No memory for msg write buf\n");
        free(eeprom_data.msgs);
        return -1;
    }
    (eeprom_data.msgs[0]).buf[0] = (unsigned char)(addr >> 8);          //addr_H
    (eeprom_data.msgs[0]).buf[1] = (unsigned char)(addr & 0xff);        //addr_L
    (eeprom_data.msgs[0]).buf[2] = data;                                //data

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
    usleep(150);

    //4.构造从eeprom读数据的消息
    eeprom_data.nmsgs = 2;
    (eeprom_data.msgs[0]).addr   = AT24C256_ADDR;
    (eeprom_data.msgs[0]).flags  = 0;             //写
    (eeprom_data.msgs[0]).len    = 2;
    (eeprom_data.msgs[0]).buf[0] = (unsigned char)(addr >> 8);          //addr_H
    (eeprom_data.msgs[0]).buf[1] = (unsigned char)(addr & 0xff);        //addr_L

    (eeprom_data.msgs[1]).addr   = AT24C256_ADDR;
    (eeprom_data.msgs[1]).flags  = I2C_M_RD; //读
    (eeprom_data.msgs[1]).len    = 1;
    (eeprom_data.msgs[1]).buf = (unsigned char *)malloc(1);
    if (NULL == (eeprom_data.msgs[1]).buf)
    {
        printf("No memory for msg read buf\n");
        free((eeprom_data.msgs[0]).buf);
        free(eeprom_data.msgs);
        return -1;
    }
    (eeprom_data.msgs[1]).buf[0] = 0;
    
    //5.使用ioctl读出数据
    ret = ioctl(fd, I2C_RDWR, (unsigned long)&eeprom_data);
    if (ret < 0)
    {
        printf("I2C read failed: %d\n", ret);
        free((eeprom_data.msgs[1]).buf);
        free((eeprom_data.msgs[0]).buf);
        free(eeprom_data.msgs);
        return ret;
    }

    printf("write data: [0x%04x], 0x%02x\n", addr, data);
    printf("read data:  [0x%04x], 0x%02x\n", addr, eeprom_data.msgs[1].buf[0]);

    //6.关闭设备
    free((eeprom_data.msgs[1]).buf);
    free((eeprom_data.msgs[0]).buf);
    free(eeprom_data.msgs);
    return close(fd);
}

int main(void)
{
    int i;
    int ret;
    int err = 0;
    int addr;
    int data;
    
    char *dev_ptr = "/dev/i2c-";
    int dev_num;
    char dev[128] = {0};

    int test_cnt;

    printf("Ipnut i2c bus id:");
    scanf("%d", &dev_num);
    sprintf(dev, "%s%d", dev_ptr, dev_num);
    printf("Use i2c bus %s\n", dev);

    printf("Ipnut test count:");
    scanf("%d", &test_cnt);

    for (i = 0; i < test_cnt; i++)
    {
        printf("test cnt:[%d]\n", i);
        printf("Ipnut eeprom addr(0x0000~0xffff):0x");
        scanf("%x", &addr);
        addr &= 0xffff;
        printf("Ipnut eeprom data(0x00~0xff):0x");
        scanf("%x", &data);
        data &= 0xff;
        ret = i2c_test(dev, addr, data);

        if (ret != 0)
            err++;
    }

    printf("test error: %d\n", err);

    return 0;
}


