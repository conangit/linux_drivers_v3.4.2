#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of_i2c.h>
#include <linux/of_gpio.h>
#include <asm/irq.h>
#include <plat/regs-iic.h>
#include <plat/iic.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach-types.h>
#include <mach/regs-gpio.h>
#include <mach/leds-gpio.h>
#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>
#include <mach/idle.h>
#include <mach/fb.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/common-smdk.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/pm.h>

// #define __DEBUG
#ifdef __DEBUG
    #define PRINTK printk
#else
    #define PRINTK(...)
#endif

#define S3C_I2C_BASEADDR 0x54000000

struct s3c_i2c_regs {
    unsigned int iiccon;
    unsigned int iicstat;
    unsigned int iicadd;
    unsigned int iiccds;
    unsigned int iiclc;
};

static struct s3c_i2c_regs *i2c_regs;

enum s3c24xx_i2c_state {
    STATE_IDLE,
    STATE_START,
    STATE_READ,
    STATE_WRITE,
    STATE_STOP
};

struct s3c_xfer_data {
    struct i2c_msg *msgs;
    int msg_num;
    int current_msg;
    int current_ptr;
    int state;
    int err;
    wait_queue_head_t wq;
};

static struct s3c_xfer_data xfer_data;



static void s3c_i2c_start(void);
static void s3c_i2c_stop(int err);

static inline int isLastMsg(void)
{
    return (xfer_data.current_msg == xfer_data.msg_num - 1);
}

static inline int isEndData(void)
{
    return (xfer_data.current_ptr >= xfer_data.msgs->len);
}

static inline int isLastData(void)
{
    return (xfer_data.current_ptr == xfer_data.msgs->len - 1);
}

static irqreturn_t i2c_handle_irq(int irq, void *dev_id)
{
    unsigned int status;

    status = i2c_regs->iicstat;
    if (status & 0x8)
        printk(KERN_ERR "%s: Bus error\n", __func__);

    switch (xfer_data.state)
    {
        case STATE_START :  /* 发出S和设备地址后 产生中断 */
        {
            PRINTK("%s: debug start\n", __func__);
            // 判断是否有ACK 没有ACK 返回错误
            if (status & S3C2410_IICSTAT_LASTBIT)
            {
                s3c_i2c_stop(-ENODEV);
                break;
            }

            if (isLastMsg() && isEndData())
            {
                s3c_i2c_stop(0);
                break;
            }

            if (xfer_data.msgs->flags & I2C_M_RD)    // read
            {
                xfer_data.state = STATE_READ;
                goto next_read;
            }
            else
            {
                xfer_data.state = STATE_WRITE;
                // break; 
                // 直接跳到写
            }
        }

        case STATE_WRITE :
        {
            PRINTK("%s: debug write\n", __func__);
            // 判断是否有ACK 没有ACK 返回错误
            if (status & S3C2410_IICSTAT_LASTBIT)
            {
                s3c_i2c_stop(-ENODEV);
                break;
            }
            
            if (!isEndData())
            {
                i2c_regs->iiccds = xfer_data.msgs->buf[xfer_data.current_ptr];
                xfer_data.current_ptr++;
                ndelay(50);
                i2c_regs->iiccon = 0xaf;
                break;
            }
            else if (!isLastMsg())
            {
                // 开始处理下一个消息
                xfer_data.msgs++;
                xfer_data.current_msg++;
                xfer_data.current_ptr = 0;
                xfer_data.state = STATE_START;
                // 开始发出S信号和设备地址
                s3c_i2c_start();
                break;
            }
            else
            {
                // 最后一个消息的最后一个数据
                s3c_i2c_stop(0);
                break;
            }
        }

        case STATE_READ :
        {
            PRINTK("%s: debug read\n", __func__);
            // 读出数据
            xfer_data.msgs->buf[xfer_data.current_ptr] = i2c_regs->iiccds;
            xfer_data.current_ptr++;
next_read:
            if (!isEndData())   // 继续发起读操作
            {
                // 最后一个数据 不发ACK
                if (isLastData())
                {
                    i2c_regs->iiccon = 0x2f;
                    break;
                }

                else
                {
                    i2c_regs->iiccon = 0xaf;
                    break;
                }
            }
            else if (!isLastMsg())  //开始处理下一条消息
            {
                // 开始处理下一个消息
                xfer_data.msgs++;
                xfer_data.current_msg++;
                xfer_data.current_ptr = 0;
                xfer_data.state = STATE_START;
                // 开始发出S信号和设备地址
                s3c_i2c_start();
                break;
            }
            else
            {
                // 最后一个消息的最后一个数据
                s3c_i2c_stop(0);
                break;
            }
        }

        default : break;
    }

    // 清中断
    i2c_regs->iiccon &= ~(S3C2410_IICCON_IRQPEND);

    return IRQ_HANDLED;
}


static void s3c_i2c_start(void)
{
    printk("%s: START\n", __func__);
    
    xfer_data.state = STATE_START;
    
    if (xfer_data.msgs->flags & I2C_M_RD)    // read
    {
        i2c_regs->iiccds  = (xfer_data.msgs->addr) << 1;
        i2c_regs->iicstat = 0xb0;
    }
    else    // write
    {
        i2c_regs->iiccds  = (xfer_data.msgs->addr) << 1;
        i2c_regs->iicstat = 0xf0;
    }
}

static void s3c_i2c_stop(int err)
{
    printk("%s: STOP\n", __func__);
    
    xfer_data.err   = err;
    PRINTK("%s: err = %d\n", __func__, err);
    
    if (xfer_data.msgs->flags & I2C_M_RD)    // read
    {
        i2c_regs->iicstat = 0x90;
        i2c_regs->iiccon  = 0xaf;
        ndelay(50);
    }
    else    // write
    {
        i2c_regs->iicstat = 0xd0;
        i2c_regs->iiccon  = 0xaf;
        ndelay(50);
    }

    // 唤醒
    xfer_data.state = STATE_STOP;
    wake_up(&xfer_data.wq);
}


static int s3c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
    unsigned long timeout;

    // msgs指向所有消息 也指向第一条消息
    xfer_data.msgs        = msgs;
    xfer_data.msg_num     = num;
    xfer_data.current_msg = 0;
    xfer_data.current_ptr = 0;
    xfer_data.err         = -ENODEV;
    
    s3c_i2c_start();

    // 休眠
    timeout = wait_event_timeout(xfer_data.wq, (xfer_data.state == STATE_STOP), HZ * 5);
    if (0 == timeout)
    {
        printk("%s: Warnning i2c timeout\n", __func__);
        return -ETIMEDOUT;
    }

    // return xfer_data.err;
    // 返回传输信息的条数
    return xfer_data.current_msg;
}

static u32 s3c_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL | I2C_FUNC_PROTOCOL_MANGLING;
}

static const struct i2c_algorithm s3c_algo = {
    .master_xfer   = s3c_master_xfer,
    .functionality = s3c_functionality,
};

static struct i2c_adapter s3c_adapter = {
    .owner = THIS_MODULE,
    .name  = "i2c-jz2440",
    .algo  = &s3c_algo,
};

static void i2c_s3c_hwinit(void)
{
    struct clk *clk;

    clk = clk_get(NULL, "i2c");
    clk_enable(clk);

    s3c_gpio_cfgpin(S3C2410_GPE(14), S3C2410_GPE14_IICSCL);
    s3c_gpio_cfgpin(S3C2410_GPE(15), S3C2410_GPE15_IICSDA);

    i2c_regs->iiccon  = (1<<7) | (0<<6) | (1<<5) | (0xf);
    i2c_regs->iicadd  = 0x10;
    i2c_regs->iicstat = 0x10;
}

static int __init i2c_s3c_bus_init(void)
{
    int res;
    
    i2c_regs = ioremap(S3C_I2C_BASEADDR, sizeof(struct s3c_i2c_regs));
    if (!i2c_regs)
    {
        printk("%s: no memory for device\n", __func__);
        return -ENOMEM;
    }

    i2c_s3c_hwinit();

    init_waitqueue_head(&xfer_data.wq);

    res = request_irq(IRQ_IIC, i2c_handle_irq, 0, "s3c2440-i2c", NULL);
    if (res)
    {
        printk("%s: request irq failed\n", __func__);
        iounmap(i2c_regs);
        return res;
    }
    
    return i2c_add_adapter(&s3c_adapter);
}

static void __exit i2c_s3c_bus_exit(void)
{
    i2c_del_adapter(&s3c_adapter);
    free_irq(IRQ_IIC, NULL);
    iounmap(i2c_regs);
}


module_init(i2c_s3c_bus_init);
module_exit(i2c_s3c_bus_exit);

MODULE_DESCRIPTION("i2c s3c2440 bus driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


