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

#define MCP4725_ADDR 0x60

#define I2C_DEV "/dev/i2c-2"

static int i2c_test(unsigned short dac_data)
{
    int fd;
    struct i2c_rdwr_ioctl_data eeprom_data;
    int ret;

    unsigned char data_H = (unsigned char)((dac_data >> 8) & 0x0f); //{0000,[11:8]}
    unsigned char data_L = (unsigned char)(dac_data & 0xff);        //[7:0]
    unsigned char mode   = (0x00 | data_H); //00 -- 快速模式 00 -- 正常模式 d11~d8 -- dac_data[11:8]
    
    //1.打开通用设备文件
    fd = open(I2C_DEV, O_RDWR);
    if (fd < 0)
    {
        printf("Open device %s failed\n", I2C_DEV);
        return -1;
    }
    printf("Open device %s...\n", I2C_DEV);

    //写-1条消息 快速写模式
    eeprom_data.msgs = (struct i2c_msg *)malloc(sizeof(struct i2c_msg));
    if (NULL == eeprom_data.msgs)
    {
        printf("No memory for msg\n");
        return -1;
    }

    //2.构造写数据到eeprom的消息
    eeprom_data.nmsgs = 1;
    (eeprom_data.msgs[0]).addr   = MCP4725_ADDR;
    (eeprom_data.msgs[0]).flags  = 0;   //写
    (eeprom_data.msgs[0]).len    = 2;

    (eeprom_data.msgs[0]).buf = (unsigned char *)malloc(2);
    if (NULL == (eeprom_data.msgs[0]).buf)
    {
        printf("No memory for msg write buf\n");
        free(eeprom_data.msgs);
        return -1;
    }
    
    (eeprom_data.msgs[0]).buf[0] = mode;          //mode & data_H
    (eeprom_data.msgs[0]).buf[1] = data_L;        //data_L

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
    //usleep(1);

    //4.关闭设备
    free((eeprom_data.msgs[0]).buf);
    free(eeprom_data.msgs);
    return close(fd);
}

int main(void)
{
    int i;
    int ret;
    int k = 0;
    int data;

    for (i = 0; i < 1; i++)
    {
        printf("test cnt:[%d]\n", i);
        printf("Ipnut dac data(0~4095):");
        scanf("%d", &data);
        data &= 0xfff;
        printf("DAC data:%d(0x%03x)\n", data, data);
        ret = i2c_test(data);

        if (ret != 0)
            k++;
    }

    printf("test error: %d\n", k);

    return 0;
}


