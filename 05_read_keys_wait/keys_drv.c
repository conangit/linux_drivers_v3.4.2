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
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/device.h>


/*
 * keys按下低电平 弹起高电平
 */

#define KEYS_MAJOR 0
#define KEYS_MINOR 0
#define KEYS_NR_DEVS 4

struct key_dev {
    int status;                     //0 - 按下 1 - 弹起
    unsigned int pin;               //key对应的GPIO
    
    unsigned int irq;               //中断向量
    unsigned long irq_flags;        //中断方式
    char irq_name[16];              //中断名称

    int ev_flag;                    //dev等待队列condition
    wait_queue_head_t wq;           //dev等待队列
    
    struct cdev cdev;               //dev字符设备
};

static struct class *keys_cls = NULL;
static struct key_dev *devs = NULL;

static unsigned int keys_major = KEYS_MAJOR;
static unsigned int keys_minor = KEYS_MINOR;
static unsigned int keys_nr_devs = KEYS_NR_DEVS;

module_param(keys_major, uint, S_IRUGO|S_IWUSR);
module_param(keys_minor, uint, S_IRUGO|S_IWUSR);

static irqreturn_t key_handle_irq(int irq, void *dev_id)
{
    struct key_dev *dev = (struct key_dev *)dev_id;
    
    dev->ev_flag = 1;
    wake_up_interruptible(&dev->wq);

    //printk(KERN_DEBUG "dev irq = %d, name = %s\n", dev->irq, dev->irq_name);

    return IRQ_HANDLED;
}

static int keys_open(struct inode *inode, struct file *filp)
{
    struct key_dev *dev;
    int ret;
    unsigned int dev_minor;
    
    dev = container_of(inode->i_cdev, struct key_dev, cdev);
    filp->private_data = dev;

    dev_minor = iminor(inode);

    if (dev_minor == keys_minor)
    {
        dev->pin = S3C2410_GPF(0);
        dev->irq = IRQ_EINT0;                       //16
        strcpy(dev->irq_name, "key S2");
        printk(KERN_DEBUG "minor: 0\n");
    }
    else if (dev_minor == keys_minor + 1)
    {
        dev->pin = S3C2410_GPF(2);
        dev->irq = IRQ_EINT2;                       //18
        strcpy(dev->irq_name, "key S3");
        printk(KERN_DEBUG "minor: 1\n");
    }
    else if (dev_minor == keys_minor + 2)
    {
        dev->pin = S3C2410_GPG(3);
        dev->irq = IRQ_EINT11;                       //55
        strcpy(dev->irq_name, "key S4");
        printk(KERN_DEBUG "minor: 2\n");
    }
    else if (dev_minor == keys_minor + 3)
    {
        dev->pin = S3C2410_GPG(11);
        dev->irq = IRQ_EINT19;                       //63
        strcpy(dev->irq_name, "key S5");
        printk(KERN_DEBUG "minor: 3\n");
    }

    dev->status = 1;                         //默认弹起
    dev->irq_flags = IRQ_TYPE_EDGE_BOTH;

    ret = request_irq(dev->irq, key_handle_irq, dev->irq_flags, dev->irq_name, dev);
    if (ret < 0)
    {
        printk(KERN_ERR "Error: request irq error(%d)!\n", ret);
    }

    printk(KERN_DEBUG "request irq success(%d).\n", ret);
    
    dev->ev_flag = 0;                  //默认阻塞打开

    return ret;
}

static int keys_release(struct inode *inode, struct file *filp)
{
    struct key_dev *dev = filp->private_data;

    free_irq(dev->irq, dev);

    return 0;
}

static ssize_t keys_read(struct file *filp, char __user *buf, size_t size, loff_t *f_ops)
{
    struct key_dev *dev = filp->private_data;
    unsigned int pinval;

    //允许等待该信号的用户空间进程可被中断 如Ctrl + C
    if (wait_event_interruptible(dev->wq, dev->ev_flag != 0))
        return -ERESTARTSYS;

    pinval = s3c2410_gpio_getpin(dev->pin);

    if (pinval)     //弹起
    {
        dev->status = 1;
    }
    else            //按下 
    {
        dev->status = 0;
    }

    if (copy_to_user(buf, &dev->status, sizeof(dev->status)))
        printk(KERN_ERR "Error:copy_to_user error!\n");

    dev->ev_flag = 0;

    return sizeof(dev->status);
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_open,
    .release = keys_release,
    .read = keys_read,
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
    if (ret)
    {
        printk(KERN_ERR "Error:adding keys %d fail!\n", index);
        return ret;
    }

    device_create(keys_cls, NULL, devno, NULL, "button%d", index);

    init_waitqueue_head(&dev->wq);

    return ret;
}

int __init keys_drv_init(void)
{
    dev_t devno;
    int ret;
    int i;

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

    devs = kmalloc(sizeof(struct key_dev) * keys_nr_devs, GFP_KERNEL);
    if (!devs)
    {
        ret = -ENOMEM;
        goto fail;
    }
    memset(devs, 0 , sizeof(struct key_dev) * keys_nr_devs);

    keys_cls = class_create(THIS_MODULE, "keys_cls");

    for (i = 0; i < keys_nr_devs; i++)
    {
        ret = key_setup(&devs[i], i);
    }

    return ret;

fail:
    unregister_chrdev_region(devno, keys_nr_devs);
    return ret;
}

void __exit keys_drv_exit(void)
{
    int i;

    if (devs)
    {
        for (i = 0; i < keys_nr_devs; i++)
        {
            device_destroy(keys_cls, MKDEV(keys_major, keys_minor + i));
            cdev_del(&devs[i].cdev);
        }
        kfree(devs);
        devs = NULL;
    }

    class_destroy(keys_cls);
    unregister_chrdev_region(MKDEV(keys_major, keys_minor), keys_nr_devs);
}

module_init(keys_drv_init);
module_exit(keys_drv_exit);

MODULE_DESCRIPTION("keys driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.1.0");



