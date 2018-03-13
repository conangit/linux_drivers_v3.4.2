#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/cdev.h>
#include <linux/device.h>



/*
 * keys按下低电平 弹起高电平
 */

#define KEYS_MAJOR 0
#define KEYS_MINOR 0
#define KEYS_NR_DEVS 4

#define GPFCON 0x56000050       //16bit
#define GPGCON 0x56000060       //32bit

struct key_dev {
    int status;         //0 - 按下 1 - 弹起
    int pin;
    unsigned long key_base_addr;
    volatile unsigned long *key_config;
    volatile unsigned long *key_data;
    struct cdev cdev;
};

struct class *keys_cls = NULL;
struct key_dev *devs = NULL;

volatile unsigned long *gpfcon  = NULL;
volatile unsigned long *gpfdata = NULL;
volatile unsigned long *gpgcon  = NULL;
volatile unsigned long *gpgdata = NULL;

unsigned int keys_major = KEYS_MAJOR;
unsigned int keys_minor = KEYS_MINOR;
unsigned int keys_nr_devs = KEYS_NR_DEVS;

module_param(keys_major, uint, S_IRUGO|S_IWUSR);
module_param(keys_minor, uint, S_IRUGO|S_IWUSR);
module_param(keys_nr_devs, uint, S_IRUGO|S_IWUSR);

static int keys_open(struct inode *inode, struct file *filp)
{
    struct key_dev *dev;
    unsigned int config_data;

    dev = container_of(inode->i_cdev, struct key_dev, cdev);
    filp->private_data = dev;
#if 0
    dev->key_config = kmalloc(sizeof(unsigned long), GFP_KERNEL);
    if (! dev->key_config)
    {
        ret = -ENOMEM;
        return ret;
    }

    dev->key_data = kmalloc(sizeof(unsigned long), GFP_KERNEL);
    if (! dev->key_data)
    {
        ret = -ENOMEM;
        return ret;
    }
#endif
    dev->key_config = (volatile unsigned long *)ioremap(dev->key_base_addr, 4);
    dev->key_data   = (volatile unsigned long *)ioremap(dev->key_base_addr + 4, 4);

    //配置GPIO为输入
    if (dev->pin == 0 || dev->pin == 2)
    {
        config_data = readw(dev->key_config);
        config_data &= ~(3 << ((dev->pin) * 2));
        writew(config_data, dev->key_config);
    }

    if (dev->pin == 3 || dev->pin == 11)
    {
        config_data = readl(dev->key_config);
        config_data &= ~(3 << ((dev->pin) * 2));
        writel(config_data, dev->key_config);
    }

    //printk(KERN_DEBUG "open: pin = %d\n", dev->pin);
    //printk(KERN_DEBUG "open: key_base_addr = 0x%p\n", dev->key_base_addr);

    return 0;
}

static int keys_release(struct inode *inode, struct file *filp)
{
    struct key_dev *dev = filp->private_data;

    iounmap(dev->key_config);
    iounmap(dev->key_data);
#if 0
    if (! dev->key_data)
    {
        kfree(dev->key_data);
        dev->key_data = NULL;
    }

    if (! dev->key_config)
    {
        kfree(dev->key_config);
        dev->key_config = NULL;
    }

    if (!devs)
    {
        kfree(devs);
        devs = NULL;
    }
#endif
    return 0;
}

static ssize_t keys__read(struct file *filp, char __user *buf, size_t size, loff_t *f_ops)
{
    struct key_dev *dev = filp->private_data;

    //printk(KERN_DEBUG "read: pin = %d\n", dev->pin);
    //printk(KERN_DEBUG "read: key_base_addr = 0x%p\n", dev->key_base_addr);

    if (dev->pin == 0 || dev->pin == 2)
        dev->status = (int)((readb(dev->key_data)) & (1 << (dev->pin)) ? 1 : 0);

    if (dev->pin == 3 || dev->pin == 11)
        dev->status = (int)((readw(dev->key_data)) & (1 << (dev->pin)) ? 1 : 0);
    
    if (copy_to_user(buf, &dev->status, sizeof(dev->status)))
        printk(KERN_ERR "Error:copy_to_user error!\n");

    return sizeof(dev->status);
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_open,
    .release = keys_release,
    .read = keys__read,
};

int key_setup(struct key_dev *dev, int index)
{   
    int ret;
    dev_t devno;

    devno = MKDEV(keys_major, keys_minor + index);
    cdev_init(&dev->cdev, &keys_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &keys_fops;
    ret = cdev_add(&dev->cdev, devno, 1);
    if (ret < 0)
    {
        printk(KERN_ERR "Error:adding keys %d fail!\n", index);
        return ret;
    }

    dev->status = 1;    //默认弹起状态
    dev->pin = index;   //紧密联系硬件 是否有相关改进方法?

    if (dev->pin == 0 || dev->pin == 2)
        dev->key_base_addr = GPFCON;

    if (dev->pin == 3 || dev->pin == 11)
        dev->key_base_addr = GPGCON;

    device_create(keys_cls, NULL, devno, NULL, "key-%d", index);

    return ret;
}

int __init keys_drv_init(void)
{
    dev_t devno;
    int ret;

    if (keys_major)
    {
        devno = MKDEV(keys_major, keys_minor);
        ret = register_chrdev_region(devno, keys_nr_devs, "keys");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, keys_minor, keys_nr_devs, "keys");
        keys_major = MAJOR(devno);
    }

    if (ret < 0)
    {
        printk(KERN_ERR "Error:Can't get keys major %d!\n", keys_major);
        return ret;
    }

    keys_cls = class_create(THIS_MODULE, "keys_cls");

    devs = kmalloc(sizeof(struct key_dev) * keys_nr_devs, GFP_KERNEL);
    if (!devs)
    {
        ret = -ENOMEM;
        goto fail;
    }

    memset(devs, 0 , sizeof(struct key_dev) * keys_nr_devs);

    ret = key_setup(&devs[0], 0);       //key1
    ret = key_setup(&devs[1], 2);       //key2
    ret = key_setup(&devs[2], 3);       //key3
    ret = key_setup(&devs[3], 11);      //key4

    return ret;

fail:
    class_destroy(keys_cls);
    unregister_chrdev_region(devno, keys_nr_devs);
    return ret;
}

void __exit keys_drv_exit(void)
{
    int i;
    
    device_destroy(keys_cls, MKDEV(keys_major, 0));
    device_destroy(keys_cls, MKDEV(keys_major, 2));
    device_destroy(keys_cls, MKDEV(keys_major, 3));
    device_destroy(keys_cls, MKDEV(keys_major, 11));

    for (i = 0; i < keys_nr_devs; i++)
        cdev_del(&devs[i].cdev);

    class_destroy(keys_cls);
    unregister_chrdev_region(MKDEV(keys_major, keys_minor), keys_nr_devs);
}

module_init(keys_drv_init);
module_exit(keys_drv_exit);

MODULE_DESCRIPTION("keys driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.0.0");



