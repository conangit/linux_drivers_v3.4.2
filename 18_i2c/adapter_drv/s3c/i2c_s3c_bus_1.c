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


static int s3c_master_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{

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
    .name  = "s3c2440",
    .algo  = &s3c_algo,
};

static int __init i2c_s3c_bus_init(void)
{
    return i2c_add_adapter(&s3c_adapter);
}

static void __exit i2c_s3c_bus_exit(void)
{
    i2c_del_adapter(&s3c_adapter);
}


module_init(i2c_s3c_bus_init);
module_exit(i2c_s3c_bus_exit);

MODULE_DESCRIPTION("i2c s3c2440 bus driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


