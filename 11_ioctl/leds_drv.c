#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/gfp.h>

#include "leds_cmd.h"


#define LEDS_MAJOR 0
#define LEDS_MINOR 0
#define LEDS_NR_DEVS 3

#define GPFCONF 0x56000050

// 注意:LED4~6位于同一寄存器
struct leds_regs {
    unsigned long gpfconf;    // 0x56000050
    unsigned long gpfdata;    // 0x56000054
};

struct led_dev {
    char *name;
    int status;
    int pin;
    struct cdev cdev;
};

struct led_dev *devs;
struct class *leds_cls;
struct leds_regs *ledsregs;

unsigned int leds_major = LEDS_MAJOR;
unsigned int leds_minor = LEDS_MINOR;
unsigned int leds_nr_devs = LEDS_NR_DEVS;

module_param(leds_major, uint, S_IRUGO | S_IWUSR);
module_param(leds_minor, uint, S_IRUGO | S_IWUSR);
module_param(leds_nr_devs, uint, S_IRUGO | S_IWUSR);

static int leds_open(struct inode *inode, struct file *filp)
{
    struct led_dev *dev;
    unsigned short config_data;
    unsigned char gpio_data;

    dev = container_of(inode->i_cdev, struct led_dev, cdev);
    filp->private_data = dev;

#if 0
    // 基于同一物理地址GPFCONF的映射 导致多次调用open()中的request_mem_region()失败
    dev->ledregs = (struct led_regs *)ioremap(GPFCONF, sizeof(struct led_regs));

    if (! request_mem_region((phys_addr_t)dev->ledregs, sizeof(struct led_regs), dev->name))
    {
        printk(KERN_ERR "Error: request_mem_region error!\n");
        goto fail;
    }
#endif

    //设置对应pin为GPIO
    config_data = ioread16(&ledsregs->gpfconf);
    config_data &= ~(3 << ((dev->pin) * 2));
    config_data |= (1 << ((dev->pin) * 2));
    iowrite16(config_data, &ledsregs->gpfconf);

    //默认熄灭led
    gpio_data = ioread8(&ledsregs->gpfdata);
    gpio_data |= (1 << (dev->pin));
    dev->status = 0;
    iowrite8(gpio_data, &ledsregs->gpfdata);

    return 0;
}


static int leds_release(struct inode *inode, struct file *filp)
{
    // 熄灭LED
    // struct led_dev *dev = filp->private_data;
    
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
    unsigned char data = ioread8(&ledsregs->gpfdata);

    if (copy_from_user(&led_data, buf, sizeof(led_data)))
        printk(KERN_ERR "copy_from_user error!\n");

    //点亮led
    if (led_data)
    {
        data &= ~(1 << (dev->pin));
        iowrite8(data, &ledsregs->gpfdata);
        dev->status = 1;
    }
    else
    {
        data |= (1 << (dev->pin));
        iowrite8(data, &ledsregs->gpfdata);
        dev->status = 0;
    }

    return sizeof(led_data);
}

static long leds_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct led_dev *dev = filp->private_data;

    unsigned char data = ioread8(&ledsregs->gpfdata);

    int ret;
    struct leds_config msg;

    /* 检测幻数 */
    if (_IOC_TYPE(cmd) != LEDS_MAGIC)
        return -EINVAL;

    /* 检测序数 */
    if (_IOC_NR(cmd) > LEDS_MAXNR)
        return -EINVAL;

#if 0
    /* copy_from_user() copy_to_user()可安全地与用户空间交换数据 */
    /* 检测访问模式 */
    if (_IOC_DIR(cmd) & _IOC_READ)
        ret = access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
        ret = access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    if (! ret)
        return -EFAULT;
#endif

    switch (cmd)
    {
        case LEDS_SET:
            ret = copy_from_user(&msg, (struct leds_config __user *)arg, sizeof(struct leds_config));
            if (ret)
                return -EINVAL;
            printk(KERN_INFO "drv:[LEDS_SET] name = %s, data = %d\n", msg.name, msg.data);

            if (msg.data)
            {
                // 点亮
                data &= ~(1 << (dev->pin));
                iowrite8(data, &ledsregs->gpfdata);
                dev->status = 1;
            }
            else
            {
                // 熄灭
                data |= (1 << (dev->pin));
                iowrite8(data, &ledsregs->gpfdata);
                dev->status = 0;
            }
            
            return 0;
            
        case LEDS_GET:
            strcpy(msg.name, dev->name);
            msg.data = dev->status;
            ret = copy_to_user((struct leds_config __user *)arg, &msg, sizeof(struct leds_config));
            if (ret)
                return -EINVAL;
            printk(KERN_INFO "drv:[LEDS_GET] name = %s, data = %d\n", msg.name, msg.data);
            return 0;

        default:
            return -EINVAL;
    }
}

static const struct file_operations leds_fops = {
    .owner = THIS_MODULE,
    .open = leds_open,
    .release = leds_release,
    .read = leds_read,
    .write = leds_write,
    .unlocked_ioctl = leds_unlocked_ioctl,
};

static int leds_setup(struct led_dev *dev, int index)
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
    device_create(leds_cls, NULL, devno, NULL, "led%d", 4 + index);

    // 需分配空间后才可使用
    dev->name = kmalloc(16, GFP_KERNEL);
    sprintf(dev->name, "%s%d", "led", 4 + index);
    dev->pin = 4 + index;   // GPF4,5,6
    dev->status = 0;        // OFF

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
        goto fail1;
    }
    memset(devs, 0, sizeof(struct led_dev) * leds_nr_devs);

    //内核自带数据结构 使用API创建时 自动分配内存空间
    leds_cls = class_create(THIS_MODULE, "leds_cls");

    // IO映射
    ledsregs = (struct leds_regs *)ioremap(GPFCONF, sizeof(struct leds_regs));

    if (! request_mem_region((phys_addr_t)ledsregs, sizeof(struct leds_regs), "leds"))
    {
        printk(KERN_ERR "Error: request_mem_region error!\n");
        goto fail2;
    }

    for (i = 0; i < leds_nr_devs; i++)
    {
        ret = leds_setup(&devs[i], i);
    }

    return ret;

fail2:
    iounmap(ledsregs);

fail1:
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

    release_mem_region((phys_addr_t)ledsregs, sizeof(struct leds_regs));
    iounmap(ledsregs);
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
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");

