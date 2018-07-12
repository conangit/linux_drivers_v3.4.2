#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>


static struct i2c_driver at24c256_driver;
static struct class *cls;
dev_t devnum;


struct at24c256 {
    struct i2c_client *client;
    struct cdev at24c256_cdev;
};

static int at24c256_open(struct inode *inode, struct file *filp)
{
    struct at24c256 *dev = container_of(inode->i_cdev, struct at24c256, at24c256_cdev);
    filp->private_data = dev;
    return 0;
}

static int at24c256_release(struct inode *inode, struct file *filp)
{
    return 0;
}

/*
 * 此驱动使用SMBUS方式进行数据的读写
 * 参照文档Documentation/i2c/smbus-protocol
 */

static ssize_t at24c256_read(struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
    struct at24c256 *dev = filp->private_data;
    unsigned char var[2];
    unsigned char addr_h, addr_l;
    unsigned char data;
    int ret;

    if (size != 3)
        return -1;

    if (copy_from_user(var, buf, 2))
        return -1;

    addr_h = var[0];
    addr_l = var[1];

    /*
     * i2c_smbus_write_byte_data()
     * S Addr Wr [A] Comm [A] Data [A] P
     *   addr        addr_h   addr_l
     *
     * i2c_smbus_read_byte()
     * S Addr Rd [A] [Data] NA P
     *   addr        data
     */

    ret = i2c_smbus_write_byte_data(dev->client, addr_h, addr_l);
    if (ret < 0)
        return -1;

    ret = i2c_smbus_read_byte(dev->client);
    if (ret < 0)
        return -1;
    else
        data = (unsigned char)ret;

    if (copy_to_user(&buf[2], &data, 1))
            return -1;

    return 3;
}

static ssize_t at24c256_write(struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    struct at24c256 *dev = filp->private_data;
    unsigned char var[3];
    unsigned char addr_h, addr_l;
    unsigned char data;
    int ret;

    if (size != 3)
        return -1;

    if (copy_from_user(var, buf, 3))
        return -1;

    addr_h = var[0];
    addr_l = var[1];
    data   = var[2];

    /*
     * i2c_smbus_write_word_data()
     * S Addr Wr [A] Comm [A] DataLow [A] DataHigh [A] P
     *   addr        addr_h   addr_l      data
     */

    ret = i2c_smbus_write_word_data(dev->client, addr_h, (data << 8) | addr_l);
    if (ret < 0)
        return -1;
    else
        return 3;
}

static struct file_operations at24c256_fops = {
    .owner   = THIS_MODULE,
    .open    = at24c256_open,
    .release = at24c256_release,
    .read    = at24c256_read,
    .write   = at24c256_write,
};

static int at24c256_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct at24c256 *dev;

    printk("%s()\n", __func__);

    dev = kmalloc(sizeof(struct at24c256), GFP_KERNEL);
    if (!dev)
    {
        printk(KERN_ERR "No memory for device\n");
        return -ENOMEM;
    }

    ret = alloc_chrdev_region(&devnum, 0, 1, "at24c256");
    if (ret < 0)
    {
        printk(KERN_ERR "Cann't alloc devnum for at24c256\n");
        kfree(dev);
        return -ENODEV;
    }

    cdev_init(&dev->at24c256_cdev, &at24c256_fops);
    dev->at24c256_cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev->at24c256_cdev, devnum, 1);
    if (ret)
    {
        printk(KERN_ERR "add cdev failed\n");
        unregister_chrdev_region(devnum, 1);
        kfree(dev);
        return ret;
    }
    
    cls = class_create(THIS_MODULE, "at24cxx");
    device_create(cls, NULL, devnum, NULL, "at24c256-%d", 0);

    dev->client = client;
    i2c_set_clientdata(client, dev);

    return 0;
}

static int at24c256_remove(struct i2c_client *client)
{
    struct at24c256 *dev = i2c_get_clientdata(client);

    printk("%s()\n", __func__);

    device_destroy(cls, devnum);
    class_destroy(cls);
    cdev_del(&dev->at24c256_cdev);
    unregister_chrdev_region(devnum, 1);
    kfree(dev);
    dev = NULL;
    
    return 0;
}

static const struct i2c_device_id at24c256_id[] = {
    { "my_at24c256", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, at24c256_id);

static struct i2c_driver at24c256_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = "i2c_at24c256_driver",
    },
    .probe     = at24c256_probe,
    .remove    = at24c256_remove,
    .id_table  = at24c256_id,
};

module_i2c_driver(at24c256_driver);


MODULE_DESCRIPTION("eeprom at24c256 driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


