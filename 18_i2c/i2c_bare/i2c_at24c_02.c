#define GPECON      (*(volatile unsigned long *)0x56000040)

#define SRCPND      (*(volatile unsigned long *)0x4a000000)
#define INTPND      (*(volatile unsigned long *)0x4a000010)
#define INTMSK      (*(volatile unsigned long *)0x4a000008)

#define IICCON      (*(volatile unsigned char*)0x54000000)
#define IICSTAT     (*(volatile unsigned char*)0x54000004)
#define IICDS       (*(volatile unsigned char*)0x5400000C)

static unsigned char DEV_ADDR = 0;
static unsigned char IIC_SPEED = 0;


static delay(unsigned int n)
{
    unsigned int i;

    while(n--)
        for(i = 0; i < 0xff; i++);
}

static inline void i2c_ack(void)
{
    while ( (IICCON & (1 << 4)) == 0 );
        delay(100);
}

static inline void i2c_nack(void)
{
    IICCON &= ~(1 << 7);
}

static inline void i2c_clear_irq(void)
{
    IICCON &= ~(1 << 4);
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

static void eeprom_write_byte(unsigned char addr, unsigned char xchar)
{
    //1.设置处理器为主模式+发送模式
    IICSTAT |= (3 << 6);

    //2.写eeprom地址到 IICDS
    IICDS = (DEV_ADDR & 0xfe);

#if 1
    //是否需要清除中断? -- 步骤2会产生中断 因为操作了IICDS
    i2c_clear_irq();
#endif

    //3.写 0xF0（M/T 起始）到 IICSTAT
    IICSTAT = 0xF0;

    //4.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //5.1.将eeprom写入字节地址写到IICDS
    IICDS = addr;

    //6.1.清除中断
    i2c_clear_irq();

    //4.1.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //5.2.将数据写入到IICDS
    IICDS = xchar;

    //6.2.清除中断
    i2c_clear_irq();

    //4.2.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //7.写 0xD0（M/T 停止）到 IICSTAT
    IICSTAT = 0xD0;
    
    //8.清除中断
    i2c_clear_irq();

    //9.等待停止条件
    delay(500);
    //while ( (iicstat & (1 << 5)) == 1 );
}

static void eeprom_random_read(unsigned char addr, unsigned char *buf, int size)
{
    int i;
    unsigned char dummy_data;
    
    //一.dummy write
    IICSTAT |= (3 << 6);
    IICDS = (DEV_ADDR & 0xfe);
    i2c_clear_irq();

    IICSTAT = 0xF0;
    i2c_ack();
    IICDS = addr;
    i2c_clear_irq();
    i2c_ack();

    //二.接收数据
    //1.设置处理器为主模式+接收模式
    IICSTAT &= ~(3 << 6);
    IICSTAT |= (0x2 << 6);

    //2.写从地址到 IICDS
    IICDS = (DEV_ADDR | 0x01);

#if 1
    //是否需要清除中断?
    i2c_clear_irq();
#endif

    //3.写 0xB0（M/T 起始）到 IICSTAT
    IICSTAT = 0xB0;

    //4.等待ACK 条件:ACK后产生中断
    i2c_ack();

#if 1
    //调试发现 第一次读取的数据无效 丢弃
    dummy_data = IICDS;
    i2c_clear_irq();
    i2c_ack();
#endif

    for (i = 0; i < size; i++)
    {
        //最后一个数据 读取完毕 给出NACK信号
        if (size-1 == i) {
            i2c_nack();
        }

        //5.从IICDS读数据
        buf[i] = IICDS;

        //6.清除中断
        i2c_clear_irq();

        //4.等待ACK 条件:ACK后产生中断
        i2c_ack();
    }

    //7.写 0x90（M/T 停止）到 IICSTAT
    IICSTAT = 0x90;

    //8.清除中断
    i2c_clear_irq();

    //9.等待停止条件
    delay(500);
    //while ( (iicstat & (1 << 5)) == 1 );
}

static void i2c_hw_init(void)
{
    //1.设置 IICCON 寄存器
    //a) 使能中断
    SRCPND |= (1 << 27);
    INTPND |= (1 << 27);
    INTMSK &= ~(1 << 27);

    IICCON |= (1 << 5);

    //b) 定义 SCL 周期
    //iicclk = pclk / 16 (or 512)
    //scl = iicclk / (IICCON[3:0]+1)
    
    //= 50000K/16/(7+1) = 400K
    //= 50000K/16/(31+1) = 100K (X)
    //= 50000K/512/(0+1) = 100K

    //大错特错呀
    //当bit6=0时 Fmin = 50_000/(16*(0xf+1)) = 195KHz
    //当bit6=1时 Fmax = 50_000/(512*(0+1)) = 100KHz

    if (IIC_SPEED)
    {
        //400Kbit/s
        IICCON &= ~(1 << 6);
        IICCON &= ~(0xf << 0);
        IICCON |= (7 << 0);
        printf("\r\nIIC bus clk 400KHz\r\n");
    }
    else
    {
        //100Kbit/s
        IICCON |= (1 << 6);
        IICCON &= ~(0xf << 0);
        IICCON |= (0 << 0);
        printf("\r\nIIC bus clk 100KHz\r\n");
    }

    //2.设置 IICSTAT 以使能串行输出
    IICSTAT |= (1 << 4);

    //3.设置GPIO为IIC引脚 此时无内部上拉 为开漏模式
    GPECON &= ~(0xf << 28);
    GPECON |= (0x2 << 28) | (0x2 << 30);

    //4.允许产生ACK
    IICCON |= (1 << 7);
}

void i2c_test(void)
{
    /* scanf bug : 必须用int类型 */
    int i2c_speed;
    int dev_addr;
    int addr;
    int w_data;
    unsigned char r_data;

    printf("\r\nIIC test...\r\n");

    printf("\r\nInput i2c bus speed [0-100Kbit/s | 1-400Kbit/s]:");
    scanf("%d", &i2c_speed);
    printf("\r\nInput I2c bus speed:%s\r\n", i2c_speed ? "400Kbit/s" : "100Kbit/s");
    IIC_SPEED = i2c_speed;

    i2c_hw_init();

    printf("\r\nInput at24c02 device addr, (ignore bit0):");
    scanf("%x", &dev_addr);
    printf("\r\nAt24c02 device addr:0x%02x\r\n", dev_addr);
    DEV_ADDR = dev_addr;

    while (1)
    {
        printf("\r\nInput eeprom write addr:");
        scanf("%x", &addr);
        printf("\r\nWrite addr:0x%02x\r\n", addr);

        printf("\r\nInput write date(char):");
        scanf("%c", &w_data);
        printf("\r\nWrite date(char):%c\r\n", w_data);
        eeprom_write_byte(addr, w_data);

        eeprom_random_read(addr, &r_data, 1);
        if ((char)r_data != (char)w_data)
            printf("\r\nRead ERROR!\r\n");
        else
            printf("\r\nRead value = %c\r\n", r_data);
    }
}


void handle_i2cirq(void)
{

}



