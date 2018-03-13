#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/io.h>
#include <linux/device.h>


int major = 0;

static struct class *keys_class_old;

volatile unsigned int *gpfcon = NULL;
volatile unsigned int *gpfdat = NULL;

volatile unsigned int *gpgcon = NULL;
volatile unsigned int *gpgdat = NULL;

static int second_drv_open(struct inode *inode, struct file *filp)
{
    unsigned short data1 = readw(gpfcon);
    unsigned int data2 = readl(gpgcon);

    data1 &= ~( (3<<(0*2)) | (3<<(2*2)) );
    writew(data1, gpfcon);

    data2 &= ~( (3<<(3*2)) | (3<<(11*2)) );
    writel(data2, gpgcon);

    return 0;
}

static ssize_t second_drv_read(struct file *filp, char __user *buf, size_t size, loff_t *loft)
{
    unsigned char key_vals[4] = {0};

    if( size != sizeof(key_vals) )
        return -EINVAL;

    key_vals[0] = ( readb(gpfdat) & (1<<0) ) ? 1 : 0;
    key_vals[1] = ( readb(gpfdat) & (1<<2) ) ? 1 : 0;

    key_vals[2] = ( readw(gpgdat) & (1<<3) ) ? 1 : 0;
    key_vals[3] = ( readw(gpgdat) & (1<<11) ) ? 1 : 0;
    
    copy_to_user(buf, key_vals, sizeof(key_vals));
    
    return sizeof(key_vals);
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = second_drv_open,
    .read = second_drv_read,
};

int second_drv_init(void)
{
    major = register_chrdev(0, "buttons", &fops);

    keys_class_old = class_create(THIS_MODULE, "keys_class_old");

    device_create(keys_class_old, NULL, MKDEV(major, 0), NULL, "buttons");

    gpfcon = (volatile unsigned int *)ioremap(0x56000050,  4);
    gpfdat = (volatile unsigned int *)ioremap(0x56000054,  4);

    gpgcon = (volatile unsigned int *)ioremap(0x56000060,  4);
    gpgdat = (volatile unsigned int *)ioremap(0x56000064,  4);

    return 0;
}

void second_drv_exit(void)
{
    iounmap(gpfcon);
    iounmap(gpfdat);
    iounmap(gpgcon);
    iounmap(gpgdat);
    device_destroy(keys_class_old, MKDEV(major, 0));
    class_destroy(keys_class_old);
    unregister_chrdev(major, "buttons");
}

module_init(second_drv_init);
module_exit(second_drv_exit);

MODULE_LICENSE("GPL");


