#define GPECON         (*(volatile unsigned long *)0x56000040)

#define SRCPND         (*(volatile unsigned long *)0x4a000000)
#define INTPND         (*(volatile unsigned long *)0x4a000010)
#define INTMSK         (*(volatile unsigned long *)0x4a000008)

#define IICCON         (*(volatile unsigned char *)0x54000000)
#define IICSTAT        (*(volatile unsigned char *)0x54000004)
#define IICDS          (*(volatile unsigned char *)0x5400000C)

static unsigned char DEV_ADDR = 0;
static unsigned char IIC_SPEED = 0;

//400MHz = 2.5ns
//400 * 2.5ns = 1us
//40000 * 2.5ns = 1ms
static udelay(unsigned int n)
{
    unsigned int i;

    while(n--)
        for(i = 0; i < 400; i++);
}

static mdelay(unsigned int n)
{
    unsigned int i;

    while(n--)
        for(i = 0; i < 400000; i++);
}

static inline void i2c_ack(void)
{
    //读到0 - 无中断 -- 等待中断的产生 
    //读到1 - 中断   -- 说明产生了ack 
    //printf("\r\nwait ack...\r\n");
    while ( (IICCON & (1 << 4)) == 0 );
    //printf("\r\nack done\r\n");
    //ack信号保持时间
    udelay(5);
}

static inline void i2c_nack(void)
{
    IICCON &= ~(1 << 7);
}

static inline void i2c_clear_irq(void)
{
    //IICCON[4]写入1 清除中断以继续
    IICCON &= ~(1 << 4);
}

static inline void i2c_master_tx(void)
{
    IICSTAT |= (3 << 6);
}

static inline void i2c_master_rx(void)
{
    IICSTAT &= ~(3 << 6);
    IICSTAT |= (0x2 << 6);
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

static void mcp4725_quick_write(unsigned short dac_data);
static void mcp4725_write_adc_eeprom(unsigned short dac_data, unsigned char eeprom);
static void mcp4725_read_status(unsigned char *buf);

//3次中断: addr(start_tx) mode(data_H) data_L
static void mcp4725_quick_write(unsigned short dac_data)
{
    unsigned char data_H;
    unsigned char data_L;
    unsigned char mode;

    if (dac_data < 0xfff)
        dac_data = dac_data + 2;    //修正
    
    data_H = (unsigned char)((dac_data >> 8) & 0x0f); //{0000,[11:8]}
    data_L = (unsigned char)(dac_data & 0xff);        //[7:0]
    mode = (0x00 | data_H); //00 -- 快速模式 00 -- 正常模式 d11~d8 -- dac_data[11:8]

    //1.设置处理器为主模式+发送模式
    i2c_master_tx();
    //2.写器件地址到IICDS
    IICDS = (DEV_ADDR & 0xfe);
    //3.写 0xF0（M/T 起始）到 IICSTAT
    IICSTAT = 0xF0;
    //4.清除中断
    i2c_clear_irq();
    //5.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //6.命令模式 关断选择 和 DAC数据高4bit
    IICDS = mode;
    i2c_clear_irq();
    i2c_ack();
    //7.写入DAC数据低8bit
    IICDS = data_L;
    i2c_clear_irq();
    i2c_ack();
    
    //8.写 0xD0（M/T 停止）到 IICSTAT
    IICSTAT = 0xD0;
    //9.清除中断
    //i2c_clear_irq();
    //10.等待停止条件
    //mdelay(1);
    udelay(50);
}

//4次中断: addr(start_tx) mode data_H data_L
static void mcp4725_write_adc_eeprom(unsigned short dac_data, unsigned char eeprom)
{
    unsigned char data_H;
    unsigned char data_L;
    unsigned char mode;

    if (dac_data < 0xfff)
        dac_data = dac_data + 2;    //修正

    data_H = (unsigned char)((dac_data >> 4) & 0xff);     //[11:4]
    data_L = (unsigned char)((dac_data & 0x0f) << 4);     //{[3:0], 0000}

    if (1 == eeprom)
    {
        //011xx00x -- 写DAC和EEPROM模式 00 -- 正常模式 0110_0000
        mode = 0x60;
    }
    else
    {
        //010xx00x -- 写DAC和EEPROM模式 00 -- 正常模式 0100_0000
        mode = 0x40;
    }

    //1.设置处理器为主模式+发送模式
    i2c_master_tx();
    //2.写器件地址到IICDS
    IICDS = (DEV_ADDR & 0xfe);
    //3.写 0xF0（M/T 起始）到 IICSTAT
    IICSTAT = 0xF0;
    //4.清除中断
    i2c_clear_irq();
    //5.等待ACK 条件:ACK后产生中断
    i2c_ack();

    //6.命令模式 关断选择
    IICDS = mode;
    i2c_clear_irq();
    i2c_ack();

    //7.写入DAC数据高8bit
    IICDS = data_H;
    i2c_clear_irq();
    i2c_ack();
    //8.写入DAC数据低4bit
    IICDS = data_L;
    i2c_clear_irq();
    i2c_ack();

    //9.写 0xD0（M/T 停止）到 IICSTAT
    IICSTAT = 0xD0;
    //10.清除中断
    //i2c_clear_irq();
    //11.等待停止条件
    udelay(100);
#if 0
    do{
        unsigned char state[5] = {0};
        mcp4725_read_status(state);
        printf("\r\nwtite eeprom state:%d\r\n", state[0] >> 7);
    }while(0);
#endif
}

//8次中断: addr(start_rx) dummy 5-data stop_rx
static void mcp4725_read_status(unsigned char *buf)
{
    int i;
    unsigned char dummy;

    //1.设置处理器为主模式+接收模式
    i2c_master_rx();
    //2.写器件地址到IICDS
    IICDS = (DEV_ADDR | 0x01);
    //3.写 0xB0（M/T 起始）到 IICSTAT
    IICSTAT = 0xB0;
    //4.清除中断
    i2c_clear_irq();
    //5.等待ACK 条件:产生中断
    i2c_ack();


    //6.从IICDS读数据
    //debug found: need dummy read
#if 1
    dummy = IICDS;
    i2c_clear_irq();
    i2c_ack();
#endif

    for (i = 0; i < 5; i++)
    {
        buf[i] = IICDS;
        i2c_clear_irq();
        i2c_ack();
    }

    //7.写 0x90（M/T 停止）到 IICSTAT
    IICSTAT = 0x90;
    //8.清除中断
    i2c_clear_irq();
    //9.等待停止条件
    //mdelay(1);
    udelay(50);
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
    int mode;
    int dac_data;
    unsigned char buf[5] = {0};
    int i;

    printf("\r\nIIC MCP4725 test...\r\n");

    printf("\r\nInput i2c bus speed [0-100Kbit/s | 1-400Kbit/s]:");
    scanf("%d", &i2c_speed);
    printf("\r\nInput I2c bus speed:%s\r\n", i2c_speed ? "400Kbit/s" : "100Kbit/s");
    IIC_SPEED = i2c_speed;

    i2c_hw_init();

    printf("\r\nInput mcp4725 device addr, (ignore bit0):");
    scanf("%x", &dev_addr);
    printf("\r\nMCP4725 device addr:0x%02x\r\n", dev_addr);
    DEV_ADDR = dev_addr;
    
    while (1)
    {
        printf("\r\nInput mcp4725 mode:\n"
            "\r0 - quick_write\n"
            "\r1 - write_adc\n"
            "\r2 - write_adc_and_eeprom\r\n"
            "\r3 - read mcp4725 status\r\n");
            
        scanf("%x", &mode);
        
        switch (mode)
        {
            case 0:
                printf("\r\nMCP4725 mode: quick_write\r\n");
                printf("\r\nInput DAC data(12bit):");
                scanf("%d", &dac_data);
                dac_data &= 0xfff;
                printf("\r\nDAC data:%d(0x%x)\r\n", dac_data, dac_data);
                mcp4725_quick_write(dac_data);
                //printf("\r\nOut value:%f V\r\n", 4.932 * (dac_data >> 12));
                break;

            case 1:
                printf("\r\nMCP4725 mode: write_adc\r\n");
                printf("\r\nInput DAC data(12bit):");
                scanf("%d", &dac_data);
                dac_data &= 0xfff;
                printf("\r\nDAC data:%d(0x%x)\r\n", dac_data, dac_data);
                mcp4725_write_adc_eeprom(dac_data, 0);
                break;

            case 2:
                printf("\r\nMCP4725 mode: write_adc_and_eeprom\r\n");
                printf("\r\nInput DAC data(12bit):");
                scanf("%d", &dac_data);
                dac_data &= 0xfff;
                printf("\r\nDAC data:%d(0x%x)\r\n", dac_data, dac_data);
                mcp4725_write_adc_eeprom(dac_data, 1);
                break;

            case 3:
                for (i = 0; i < 5; i++) {
                    buf[i] = 0;
                    udelay(1);
                }
                printf("\r\nMCP4725 mode: read mcp4725 status\r\n");
                mcp4725_read_status(buf);
                printf("\r\nMCP4725 get mode: busy:%d, POR:%d, PD:%d\r\n", buf[0]>>7, (buf[0]&(1<<6))>>6, (buf[0]&(3<<1))>>1);
                printf("\r\nMCP4725 get adc: %d\r\n", (buf[1] << 4) | (buf[2] >> 4));
                printf("\r\nMCP4725 get eeprom mode: PD:%d\r\n", (buf[3] & 0x60) >> 1);
                printf("\r\nMCP4725 get eeprom adc: %d\r\n", ((buf[3] & 0xf) << 8) | buf[4]);
                break;

            default:
                printf("\r\nMCP4725 mode: ERROR!\r\n");
                break;
        }
    }
}


void handle_i2cirq(void)
{
#if 0
    static unsigned int i = 1;
    printf("\r\n[%u]\r\n", i++);
#endif
}



//着重调试了 在Tx和Rx情况下 中断发生的时机
//
//调试发现 有时必须执行了mcp4725_write_adc_eeprom(dac_data, 0/1)才可以多次执行mcp4725_read_status

