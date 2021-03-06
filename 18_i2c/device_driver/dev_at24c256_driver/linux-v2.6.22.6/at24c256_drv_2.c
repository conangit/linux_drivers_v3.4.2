#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>



static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };
static unsigned short forces_addr[] = {ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short * forces[] = {forces_addr, NULL};

#if 0
/* Addresses to scan */
static unsigned short normal_addr[] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, I2C_CLIENT_END };
#endif

static struct i2c_client_address_data addr_data = {
    // 要发出S信号和地址信号并得到ACK 才能确认是否存在这个设备
    .normal_i2c = normal_addr,
    .probe      = ignore,
    .ignore     = ignore,
    // 强制认为存在这个设备
    // .forces     = forces,
};

static struct i2c_driver at24c256_driver;
static struct class *cls;
dev_t devnum;

struct at24c256_dev {
    struct i2c_client at24c256_client;
    struct cdev at24c256_cdev;
};

static ssize_t at24c256_read(struct file *filp, char __user *buf, size_t size, loff_t *offset)
{

    return 0;
}

static ssize_t at24c256_write(struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{

    return 0;
}


static struct file_operations at24c256_fops = {
    .owner = THIS_MODULE,
    .read  = at24c256_read,
    .write = at24c256_write,
};


static int at24c256_probe(struct i2c_adapter *adap, int addr, int kind)
{
    struct at24c256_dev *dev;
    int ret;
    
    printk("%s()\n", __func__);

    dev = kzalloc(sizeof(struct at24c256_dev), GFP_KERNEL);
    dev->at24c256_client.addr = addr;
    dev->at24c256_client.adapter = adap;
    dev->at24c256_client.driver = &at24c256_driver;
    strcpy(dev->at24c256_client.name, "at24c256");
    i2c_attach_client(&dev->at24c256_client);
    i2c_set_clientdata(&dev->at24c256_client, dev);

    ret = alloc_chrdev_region(&devnum, 0, 1, "at24c256");
    if (ret <0)
    {
        printk(KERN_ERR "Cann't alloc devnum for at24c256\n");
        goto error0;
    }

    cdev_init(&dev->at24c256_cdev, &at24c256_fops);
    dev->at24c256_cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev->at24c256_cdev, devnum, 1);

    cls = class_create(THIS_MODULE, "at24cxx");
    class_device_create(cls, NULL, devnum, NULL, "at24c256-%d", 0);
    
    return 0;

error0:
    i2c_detach_client(&dev->at24c256_client);
    kfree(i2c_get_clientdata(&dev->at24c256_client));
    return ret;
}

static int at24c256_attach(struct i2c_adapter *adap)
{
    return i2c_probe(adap, &addr_data, at24c256_probe);
}

static int at24c256_detach(struct i2c_client *client)
{
    int err;
    struct at24c256_dev *dev;
    
    printk("%s()\n", __func__);

    dev = container_of(client, struct at24c256_dev, at24c256_client);

    class_device_destroy(cls, devnum);
    class_destroy(cls);
    cdev_del(&dev->at24c256_cdev);
    unregister_chrdev_region(devnum, 1);

    err = i2c_detach_client(client);
    if (err)
        return err;

    kfree(i2c_get_clientdata(client));
    
    return 0;
}


static struct i2c_driver at24c256_driver = {
    .driver = {
        .name   = "at24c256",
    },
    // <linux/i2c-id.h>
    // .id = 
    .attach_adapter = at24c256_attach,
    .detach_client  = at24c256_detach,
};


static int __init at24c256_init(void)
{
    return i2c_add_driver(&at24c256_driver);
}

static void __exit at24c256_exit(void)
{
    i2c_del_driver(&at24c256_driver);
}


module_init(at24c256_init);
module_exit(at24c256_exit);

MODULE_DESCRIPTION("eeprom at24c256 driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


