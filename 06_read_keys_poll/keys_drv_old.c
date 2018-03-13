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
#include <linux/poll.h>


static int keys_major = 0;
static struct class *keys_old_cls =NULL;

static int ev_press = 0;
static DECLARE_WAIT_QUEUE_HEAD(buttons_wq);

static int key_val = 0;

struct pin_desc{
    unsigned int pin;
    unsigned int key_val;
};

/*
 * 按下  0x01,0x02,0x03,0x04
 * 弹起  0x81,0x82,0x83,0x84
 */
struct pin_desc pins_desc[4] = {
    {S3C2410_GPF(0),   0x01},
    {S3C2410_GPF(2),   0x02},
    {S3C2410_GPG(3),   0x03},
    {S3C2410_GPG(11),  0x04},
};

static irq_handler_t buttons_irq(int irq, void *dev_id)
{
    struct pin_desc *pindesc = (struct pin_desc*)dev_id;

    unsigned int pinval = s3c2410_gpio_getpin(pindesc->pin);

    if(pinval)              //弹起
    {
        key_val = 0x80 | (pindesc->key_val);
    }
    else                    //按下
    {
        key_val = pindesc->key_val;
    }

    ev_press = 1;
    wake_up_interruptible(&buttons_wq);
    
    return IRQ_HANDLED;
}

static int keys_open(struct inode *inode, struct file *filp)
{
    int ret;

    ret = request_irq(IRQ_EINT0,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S2", (void *)&pins_desc[0]);
    ret = request_irq(IRQ_EINT2,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S3", (void *)&pins_desc[1]);
    ret = request_irq(IRQ_EINT11, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S4", (void *)&pins_desc[2]);
    ret = request_irq(IRQ_EINT19, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S5", (void *)&pins_desc[3]);

    return ret;
}

static ssize_t keys_read(struct file *filp, char __user *buf, size_t size, loff_t *loft)
{
    if( size != 1)
        return -EINVAL;

//似乎变得可有可无
//    if (wait_event_interruptible(buttons_wq, ev_press != 0))
//        return -ERESTARTSYS;

    if (copy_to_user(buf, &key_val, 1))
        printk(KERN_ERR "copy_to_user error!\n");
    
    ev_press = 0;
    
    return 1;
}

static int keys_close(struct inode *inode, struct file *filp)
{
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    return 0;
}

static unsigned int keys_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    //将进程挂入buttons_wq等待队列
    poll_wait(filp, &buttons_wq, wait);

    if (ev_press != 0)
        mask |= POLLIN | POLLRDNORM;        //设备可读
    
    return mask;
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_open,
    .read = keys_read,
    .release = keys_close,
    .poll = keys_poll,
};

static int __init keys_drv_init(void)
{
    keys_major = register_chrdev(0, "keys_old_drv", &keys_fops);
    keys_old_cls = class_create(THIS_MODULE, "keys_old_cls");
    device_create(keys_old_cls, NULL, MKDEV(keys_major, 0), NULL, "buttons");

    return 0;
}

static void __exit keys_drv_exit(void)
{
    device_destroy(keys_old_cls, MKDEV(keys_major, 0));
    class_destroy(keys_old_cls);
    unregister_chrdev(keys_major, "forth_drv");
}

module_init(keys_drv_init);
module_exit(keys_drv_exit);

MODULE_LICENSE("GPL");


