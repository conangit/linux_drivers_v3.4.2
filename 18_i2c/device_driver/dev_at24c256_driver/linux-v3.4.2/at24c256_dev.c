#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/gfp.h>


static int adap_id = 0;
module_param(adap_id, int, 0644);
MODULE_PARM_DESC(adap_id, " i2c_get_adapter(adap_id), default:0");


struct at24c256_dev {
     struct i2c_adapter *adap;
     struct i2c_client *client;
};

static struct at24c256_dev *dev;

#if 0
static struct i2c_board_info at24c256_info = {
    I2C_BOARD_INFO("my_at24c256", 0x50),
    // I2C_BOARD_INFO("my_at24c256", 0x51),
};
#else
unsigned short addr_list[] = { 0x51, 0x50, I2C_CLIENT_END };
static struct i2c_board_info at24c256_info;
#endif

static int __init at24c256_dev_init(void)
{
    dev = kmalloc(sizeof(struct at24c256_dev), GFP_KERNEL);
    if (!dev) {
        printk(KERN_ERR "%s: no memcopy for at24c256 device\n", __func__);
        return -ENOMEM;
    }

    dev->adap = i2c_get_adapter(adap_id);
    if (!dev->adap) {
        printk(KERN_ERR "%s: i2c_get_adapter %d failed\n", __func__, adap_id);
        kfree(dev);
        return -EINVAL;
    }

    /*
     * i2c_new_device()强制认为设备存在
     * i2c_new_probed_device()对于已经识别的设备 才会创建设备
     */
#if 0
    // dev->client = i2c_new_device(dev->adap, &at24c256_info);
#else
    memset(&at24c256_info, 0, sizeof(struct i2c_board_info));
    strlcpy(at24c256_info.type, "my_at24c256", I2C_NAME_SIZE);
    dev->client = i2c_new_probed_device(dev->adap, &at24c256_info, addr_list, NULL);
#endif

    if (!dev->client) {
        printk(KERN_ERR "%s: i2c_new_device()failed\n", __func__);
        i2c_put_adapter(dev->adap);
        kfree(dev);
        return -EINVAL;
    }

    i2c_set_clientdata(dev->client, dev);
    i2c_set_adapdata(dev->adap, dev);

    i2c_put_adapter(dev->adap);

    return 0;
}

static void __exit at24c256_dev_exit(void)
{
    i2c_unregister_device(dev->client);
    // i2c_put_adapter(dev->adap);
    kfree(dev);
    dev = NULL;
}

module_init(at24c256_dev_init);
module_exit(at24c256_dev_exit);

MODULE_DESCRIPTION("eeprom at24c256 device");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


