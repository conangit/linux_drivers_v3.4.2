#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>


#define GPECON  0x56000040
#define SRCPND  0X4A000000
#define INTPND  0X4A000010
#define INTMSK  0X4A000008

#define IICCON  0x54000000
#define IICSTAT 0x54000004
#define IICDS   0x5400000C


volatile unsigned long *gpecon;
volatile unsigned long *srcpnd;
volatile unsigned long *intpnd;
volatile unsigned long *intmsk;

volatile unsigned char *iiccon;
volatile unsigned char *iicstat;
volatile unsigned char *iicds;

#define AT24C02_W_ADDR 0xA0
#define AT24C02_R_ADDR 0xA1

static void disable_i2c_irq(void)
{
    unsigned int l_data;
    
    l_data = ioread32(srcpnd);
    l_data |= (1 << 27);
    iowrite32(l_data, srcpnd);

    l_data = ioread32(intpnd);
    l_data |= (1 << 27);
    iowrite32(l_data, intpnd);
}

static inline void i2c_ack(void)
{
    while ( (ioread8(iiccon) & (1 << 4)) == 0 );
        mdelay(1);
}

#if 0
//参考samsung s3c2440使用手册
static void s3c2440_write_byte(void)
{
    //1.设置处理器为主模式+发送模式
    //2.写从地址到 IICDS
    //3.写 0xF0（M/T 起始）到 IICSTAT
    //4.等待ACK 条件:ACK后产生中断
    
    //5.将要传输的数据写入IICDS
    //6.清除中断
    //4.等待ACK 条件:ACK后产生中断
    
    //7.写 0xD0（M/T 停止）到 IICSTAT
    //8.清除中断
    //9.等待停止条件
}

static void s3c2440_read_byte(void)
{
    //1.设置处理器为主模式+接收模式
    //2.写从地址到 IICDS
    //3.写 0xB0（M/T 起始）到 IICSTAT
    //4.等待ACK 条件:ACK后产生中断
    
    //5.从IICDS读数据
    //6.清除中断
    //4.等待ACK 条件:ACK后产生中断
    
    //7.写 0x90（M/T 停止）到 IICSTAT
    //8.清除中断
    //9.等待停止条件
}
#endif

static void at24c02_write_byte(unsigned char addr, unsigned char xchar)
{
    unsigned char tmp_data;
    
    //1.设置处理器为主模式+发送模式
    tmp_data = ioread8(iicstat);
    tmp_data |= (3 << 6);
    iowrite8(tmp_data, iicstat);

    printk("%s() 1\n", __func__);
    
    //2.写eeprom地址到 IICDS
    iowrite8(AT24C02_W_ADDR, iicds);

    printk("%s() 2\n", __func__);

    //是否需要清除中断?
    tmp_data = ioread8(iiccon);
    tmp_data &= ~(1 << 4);
    iowrite8(tmp_data, iiccon);
    disable_i2c_irq();
    
    //3.写 0xF0（M/T 起始）到 IICSTAT
    iowrite8(0xF0, iicstat);

    printk("%s() 3\n", __func__);
    
    //4.等待ACK 条件:ACK后产生中断
    i2c_ack();

    printk("%s() 4\n", __func__);

    //5.1.将eeprom写入字节地址写到IICDS
    iowrite8(addr, iicds);

    //6.1.清除中断
    tmp_data = ioread8(iiccon);
    tmp_data &= ~(1 << 4);
    iowrite8(tmp_data, iiccon);
    disable_i2c_irq();

    //4.1.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //5.2.将数据写入到IICDS
    iowrite8(xchar, iicds);

    //6.2.清除中断
    tmp_data = ioread8(iiccon);
    tmp_data &= ~(1 << 4);
    iowrite8(tmp_data, iiccon);
    disable_i2c_irq();

    //4.2.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //7.写 0xD0（M/T 停止）到 IICSTAT
    iowrite8(0xD0, iicstat);
    
    //8.清除中断
    tmp_data = ioread8(iiccon);
    tmp_data &= ~(1 << 4);
    iowrite8(tmp_data, iiccon);
    disable_i2c_irq();
    
    //9.等待停止条件
    mdelay(5);
    //while ( (ioread8(iicstat) & (1 << 5)) == 1 );
}

static void at24c02_random_read(unsigned char addr, unsigned char *buf, int size)
{
    unsigned char tmp_data;
    int i;
    
    //一.dummy write
    tmp_data = ioread8(iicstat);
    tmp_data |= (3 << 6);
    iowrite8(tmp_data, iicstat);
    
    iowrite8(AT24C02_W_ADDR, iicds);
    iowrite8(0xF0, iicstat);
    i2c_ack();
    iowrite8(addr, iicds);
    i2c_ack();

    //二.接收数据
    //1.设置处理器为主模式+接收模式
    tmp_data = ioread8(iicstat);
    tmp_data &= ~(3 << 6);
    tmp_data |= (0x2 << 6);
    iowrite8(tmp_data, iicstat);

    //2.写从地址到 IICDS
    iowrite8(AT24C02_R_ADDR, iicds);

    //是否需要清除中断?

    //3.写 0xB0（M/T 起始）到 IICSTAT
    iowrite8(0xB0, iicstat);

    //4.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //调试发现 第一次读取的数据无效 丢弃
    ioread8(iicds);
    tmp_data = ioread8(iiccon);
    tmp_data &= ~(1 << 4);
    iowrite8(tmp_data, iiccon);
    disable_i2c_irq();
    i2c_ack();

    for (i = 0; i < size; i++)
    {
        //最后一个数据 读取完毕 给出NACK信号
        if (size - 1 == i)
        {
            tmp_data = ioread8(iiccon);
            tmp_data &= ~(1 << 7);
            iowrite8(tmp_data, iiccon);
        }

        //5.从IICDS读数据
        buf[i] = ioread8(iicds);

        //6.清除中断
        tmp_data = ioread8(iiccon);
        tmp_data &= ~(1 << 4);
        iowrite8(tmp_data, iiccon);
        disable_i2c_irq();

        //4.等待ACK 条件:ACK后产生中断
        i2c_ack();
    }

    //7.写 0x90（M/T 停止）到 IICSTAT
    iowrite8(0x90, iicstat);

    //8.清除中断
    tmp_data = ioread8(iiccon);
    tmp_data &= ~(1 << 4);
    iowrite8(tmp_data, iiccon);
    disable_i2c_irq();

    //9.等待停止条件
    mdelay(5);
    //while ( (ioread8(iicstat) & (1 << 5)) == 1 );
}

static void i2c_hw_init(void)
{
    unsigned int l_data;
    unsigned char c_data;
    //ioremap
    gpecon = (volatile unsigned long*)ioremap(GPECON, 4);
    srcpnd = (volatile unsigned long*)ioremap(SRCPND, 4);
    intpnd = (volatile unsigned long*)ioremap(INTPND, 4);
    intmsk = (volatile unsigned long*)ioremap(INTMSK, 4);

    iiccon = (volatile unsigned char*)ioremap(IICCON, 1);
    iicstat = (volatile unsigned char*)ioremap(IICSTAT, 1);
    iicds = (volatile unsigned char*)ioremap(IICDS, 1);

    //1.设置 IICCON 寄存器
    //a) 使能中断
    l_data = ioread32(srcpnd);
    l_data |= (1 << 27);
    iowrite32(l_data, srcpnd);

    l_data = ioread32(intpnd);
    l_data |= (1 << 27);
    iowrite32(l_data, intpnd);

    l_data = ioread32(intmsk);
    l_data &= ~(1 << 27);
    iowrite32(l_data, intmsk);

    c_data = ioread8(iiccon);
    c_data |= (1 << 5);
    iowrite8(c_data, iiccon);

    //b) 定义 SCL 周期
    //iicclk = pclk / 16
    //scl = iicclk / (IICCON[3:0]+1)
    //≈100KHz
    c_data = ioread8(iiccon);
    c_data &= ~(1 << 6);
    c_data &= ~(0xf << 0);
    c_data |= (30 << 0);
    iowrite8(c_data, iiccon);

    //2.设置 IICSTAT 以使能串行输出
    c_data = ioread8(iicstat);
    c_data |= (1 << 4);
    iowrite8(c_data, iicstat);

    //3.设置GPIO
    l_data = ioread32(gpecon);
    l_data &= ~(0xf << 28);
    l_data |= (0x2 << 28) | (0x2 << 30);
    iowrite32(l_data, gpecon);

    //4.允许产生ACK
    c_data = ioread8(iiccon);
    c_data |= (1 << 7);
    iowrite8(c_data, iiccon);
}

static void i2c_hw_exit(void)
{
    iounmap(iicds);
    iounmap(iicstat);
    iounmap(iiccon);
    iounmap(intmsk);
    iounmap(intpnd);
    iounmap(srcpnd);
    iounmap(gpecon);
}

static void test(void)
{
    int i;
    unsigned char data[256] = {0};

    printk("here_1\n");
    at24c02_write_byte(0x23, 'A');
    printk("here_2\n");
    at24c02_write_byte(0x24, 'B');
    at24c02_write_byte(0x25, 'C');
    at24c02_write_byte(0x26, 'D');
    
    at24c02_random_read(0x23, data, 4);
    printk("read data: \n");
    for (i = 0; i < 4; i++)
        printk("%c \n", data[i]);
    printk("\n");
}

static int __init i2c_init(void)
{
    i2c_hw_init();
    test();
    
    return 0;
}

static void __exit i2c_exit(void)
{
    i2c_hw_exit();
}

module_init(i2c_init);
module_exit(i2c_exit);

MODULE_DESCRIPTION("bare i2c module");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


