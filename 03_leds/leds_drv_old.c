#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/io.h>
#include <asm/io.h>

int major = 0;

static struct class *leds_cls_old;

volatile unsigned int *gpfcon = NULL;
volatile unsigned int *gpfdat = NULL;


static int leds_drv_open(struct inode *node, struct file *filp)
{
    unsigned short conf_data;

    printk("call func: %s()\n", __FUNCTION__);
    
    gpfcon = (volatile unsigned int *)ioremap(0x56000050, 4);
    gpfdat = (volatile unsigned int *)ioremap(0x56000054, 4);

    // *gpfcon &= ~( (3<<(4*2)) | (3<<(5*2)) | (3<<(6*2)) ); 
    // *gpfcon |= ( (1<<(4*2)) | (1<<(5*2)) | (1<<(6*2)) ); 

    conf_data = readw(gpfcon);
    conf_data &= ~( (3<<(4*2)) | (3<<(5*2)) | (3<<(6*2)) );
    conf_data |= ( (1<<(4*2)) | (1<<(5*2)) | (1<<(6*2)) );
    writew(conf_data, gpfcon);

    return 0;
}

static ssize_t leds_drv_write(struct file *filp, const char __user * buf, size_t size, loff_t *loft)
{
    int val = 0;
    unsigned char data = readb(gpfdat);

    printk("call func: %s()\n", __FUNCTION__);

    copy_from_user(&val, buf, size);

    if(0 == val)
    {
        data &= (~((1<<4) | (1<<5) | (1<<6)));
        writeb(data, gpfdat);
    }
    else
    {
        data |= ((1<<4) | (1<<5) | (1<<6));
        writeb(data, gpfdat);
    }
    
    return 0;
}

static int leds_release(struct inode *node, struct file *filp)
{
    printk("call func: %s()\n", __FUNCTION__);
    
    iounmap(gpfcon);
    iounmap(gpfdat);
    
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = leds_drv_open,
    .write = leds_drv_write,
    .release = leds_release,
    .release = leds_release,
};

int __init leds_drv_init(void)
{
    printk("call func: %s()\n", __FUNCTION__);
    
    major = register_chrdev(0, "leds_drv", &fops);

    leds_cls_old = class_create(THIS_MODULE, "leds_cls_old");

    device_create(leds_cls_old, NULL, MKDEV(major, 0), NULL, "leds");

    return 0;
}

void __exit leds_drv_exit(void)
{
    printk("call func: %s()\n", __FUNCTION__);
    
    device_destroy(leds_cls_old, MKDEV(major, 0));
    class_destroy(leds_cls_old);
    unregister_chrdev(major, "leds_drv");
}

module_init(leds_drv_init);
module_exit(leds_drv_exit);

MODULE_DESCRIPTION("leds driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.0.0");


