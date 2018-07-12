#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>


static int demo_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    printk(KERN_ERR "%s: probed\n", __func__);
    return 0;
}

static int demo_remove(struct i2c_client *client)
{
    printk(KERN_ERR "%s: remove\n", __func__);
    return 0;
}

static int demo_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    if (0x50 == client->addr) {
        printk(KERN_ERR "%s: addr:0x%x, adap: %d\n", __func__, client->addr, client->adapter->nr);
        strlcpy(info->type, "at24c256", I2C_NAME_SIZE);
    }

    if (0x60 == client->addr) {
        printk(KERN_ERR "%s: addr:0x%x, adap: %d\n", __func__, client->addr, client->adapter->nr);
        strlcpy(info->type, "at24c256", I2C_NAME_SIZE);
    }

    return 0;
}

static const unsigned short addr_list[] = {0x50, 0x60, I2C_CLIENT_END};

static const struct i2c_device_id demo_id[] = {
    { "at24c256", 0 },
    { "mcp4725", 1},
    {}
    /* 这里只做例子来驱动测试 真实的驱动不会把两类毫不相关的设备依附在一个驱动上 */
};

MODULE_DEVICE_TABLE(i2c, demo_id);

static struct i2c_driver demo_driver = {
    /* 驱动相关 */
    .driver = {
        .owner = THIS_MODULE,
        .name  = "i2c_demo_driver",
    },
    .probe     = demo_probe,
    .remove    = demo_remove,
    .id_table  = demo_id,

    /* 设备相关 */
    .class        = I2C_CLASS_HWMON,
    .detect       = demo_detect,
    .address_list = addr_list,
};

module_i2c_driver(demo_driver);


MODULE_DESCRIPTION("use i2c_driver detect() api to bind i2c_client to i2c_adapter");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


/* log

./i2cdetect -y 0
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: 60 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --

./i2cdetect -y 2
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- -- 

insmod driver_device.ko 
demo_detect: addr:0x50, adap: 2
demo_probe: probed
demo_detect: addr:0x60, adap: 0
demo_probe: probed

ls /sys/bus/i2c/devices/
0-0060  2-0050  i2c-0   i2c-1   i2c-2

*/


