#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <asm/io.h>
#include <linux/slab.h>
#include <linux/gfp.h>

#define LEDS_MAJOR 0
#define LEDS_MINOR 0
#define LEDS_NR_DEVS 3
#define LEDS_BASE_ADDR 4

struct led_dev {
    int status;
    int pin;
    struct cdev cdev;
};

struct led_dev *devs;
struct class *leds_cls;

volatile unsigned long *gpfcon;
volatile unsigned long *gpfdata;

int leds_major = LEDS_MAJOR;
int leds_minor = LEDS_MINOR;
int leds_nr_devs = LEDS_NR_DEVS;

module_param(leds_major, int, S_IRUGO | S_IWUSR);
module_param(leds_minor, int, S_IRUGO | S_IWUSR);
module_param(leds_nr_devs, int, S_IRUGO | S_IWUSR);

static int leds_open(struct inode *inode, struct file *filp)
{
    struct led_dev *dev;
    unsigned short config_data;
    unsigned char gpio_data;
    unsigned int dev_major;
    unsigned int dev_minor;

    //将"设备"保存在file结构中 方便今后(read write...)对该"设备"的访问
    dev = container_of(inode->i_cdev, struct led_dev, cdev);
    filp->private_data = dev;

    gpfcon = (volatile unsigned long *)ioremap(0x56000050, 4);
    gpfdata = (volatile unsigned long *)ioremap(0x56000054, 4);

#if 1
    //通过次设备号 确定打开的设备为xxx
    dev_major = imajor(inode);
    dev_minor = iminor(inode);
    printk(KERN_DEBUG "device major = %u, minor = %u\n", dev_major, dev_minor);
#endif

    //设置为GPIO
    config_data = readw(gpfcon);
    config_data &= ~(3 << ((dev->pin) * 2));
    config_data |= (1 << ((dev->pin) * 2));
    writew(config_data, gpfcon);

    //默认熄灭led
    gpio_data = readb(gpfdata);
    gpio_data |= (1 << (dev->pin));
    dev->status = 0;
    writeb(gpio_data, gpfdata);

    return 0;
}


static int leds_release(struct inode *inode, struct file *filp)
{
    struct led_dev *dev = filp->private_data;
    unsigned char gpio_data = readb(gpfdata);

#if 0
    gpio_data = readb(gpfdata);
    gpio_data |= (1 << (dev->pin));
    dev->status = 0;
    writeb(gpio_data, gpfdata);
#endif

    iounmap(gpfdata);
    iounmap(gpfcon);

    return 0;
}

static ssize_t leds_read(struct file *filp, char __user *buf, size_t size, loff_t *f_ops)
{
    struct led_dev *dev = filp->private_data;

    if (copy_to_user(buf, &dev->status, sizeof(dev->status)))
        printk(KERN_ERR "copy_to_user error!\n");

    return sizeof(dev->status);
}

static ssize_t leds_write(struct file *filp, const char __user *buf, size_t size, loff_t *f_ops)
{
    struct led_dev *dev = filp->private_data;
    int led_data;
    unsigned char gpio_data = readb(gpfdata);

    if (copy_from_user(&led_data, buf, sizeof(led_data)))
        printk(KERN_ERR "copy_from_user error!\n");

    //点亮led
    if (led_data == 1)
    {
        gpio_data &= ~(1 << (dev->pin));
        dev->status = 1;
        writeb(gpio_data, gpfdata);
    }
    else
    {
        gpio_data |= (1 << (dev->pin));
        dev->status = 0;
        writeb(gpio_data, gpfdata);
    }

    return sizeof(led_data);
}

static const struct file_operations leds_fops = {
    .owner = THIS_MODULE,
    .open = leds_open,
    .release = leds_release,
    .read = leds_read,
    .write = leds_write,
};


int leds_setup(struct led_dev *dev, int index)
{
    int ret = 0;
    dev_t devno;

    devno = MKDEV(leds_major, leds_minor + index);
    cdev_init(&dev->cdev, &leds_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &leds_fops;
    ret = cdev_add(&dev->cdev, devno, 1);
    if (ret < 0)
    {
        printk(KERN_ERR "Can't add chrdev %d\n", index);
    }
    device_create(leds_cls, NULL, devno, NULL, "led-%d", index);

    dev->pin = LEDS_BASE_ADDR + index;
    dev->status = 0;

    return ret;
}


int __init leds_drv_init(void)
{
    dev_t devno;
    int ret;
    int i;
    
    if (leds_major)
    {
        devno = MKDEV(leds_major, leds_minor);
        ret = register_chrdev_region(devno, leds_nr_devs, "leds_driver");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, leds_minor, leds_nr_devs, "leds_driver");
        leds_major = MAJOR(devno);
    }

    if (ret < 0)
    {
        printk(KERN_ERR "Can't get leds major %d!\n", leds_major);
    }

    devs = kmalloc(sizeof(struct led_dev) * leds_nr_devs, GFP_KERNEL);
    if (!devs)
    {
        ret = -ENOMEM;
        goto fail;
    }
    memset(devs, 0, sizeof(struct led_dev) * leds_nr_devs);

    //内核自带数据结构 使用API创建时 自动分配内存空间
    leds_cls = class_create(THIS_MODULE, "leds_cls");

    for (i = 0; i < leds_nr_devs; i++)
    {
        ret = leds_setup(&devs[i], i);
    }

    return ret;

fail:
    unregister_chrdev_region(devno, leds_nr_devs);
    return ret;
}

void __exit leds_drv_exit(void)
{
    int i;
    dev_t devno;

    for (i = 0; i < leds_nr_devs; i++)
    {
        devno = MKDEV(leds_major, leds_minor + i);
        device_destroy(leds_cls, devno);
        cdev_del(&devs[i].cdev);
    }

    class_destroy(leds_cls);

    if (devs)
    {
        kfree(devs);
        devs = NULL;
    }

    devno = MKDEV(leds_major, leds_minor);
    unregister_chrdev_region(devno, leds_nr_devs);
}

module_init(leds_drv_init);
module_exit(leds_drv_exit);

MODULE_DESCRIPTION("leds driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.0.0");


