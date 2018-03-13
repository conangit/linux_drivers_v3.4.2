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
#include <linux/fcntl.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/timer.h>



int major = 0;
static struct class *keys_old_cls;

struct pin_desc{
    unsigned int pin;
    unsigned int key_val;
};

int key_val = 0;

static volatile int ev_press = 0;
static DECLARE_WAIT_QUEUE_HEAD(buttons_wq);

static struct fasync_struct *button_fasync;

static DEFINE_MUTEX(buttons_lock);

static struct timer_list buttons_timer;

/*
 * 按下: 0x01,0x02,0x03,0x04
 * 弹起: 0x81,0x82,0x83,0x84
 */

struct pin_desc pins_desc[4] = {
    {S3C2410_GPF(0),  0x01},
    {S3C2410_GPF(2),  0x02},
    {S3C2410_GPG(3),  0x03},
    {S3C2410_GPG(11), 0x04},
};

struct pin_desc* irq_pd = NULL;

static irq_handler_t buttons_irq(int irq, void *dev_id)
{
    irq_pd = (struct pin_desc*)dev_id;
    
    mod_timer(&buttons_timer, jiffies + HZ/50);    //20ms
    
    return IRQ_HANDLED;
}

static void buttons_timer_function(unsigned long data)
{
    struct pin_desc* pindesc = irq_pd;
    unsigned int pinval;

    if(pindesc != NULL)
    {
        pinval = s3c2410_gpio_getpin(pindesc->pin);

        if(pinval)
        {
            key_val = 0x80 | (pindesc->key_val);
        }
        else
        {
            key_val = pindesc->key_val;
        }

        ev_press = 1;
        wake_up_interruptible(&buttons_wq);

        kill_fasync(&button_fasync, SIGIO, POLL_IN);
    }
}

static int keys_drv_open(struct inode *inode, struct file *filp)
{
    if ((filp->f_flags) & O_NONBLOCK)
    {
        if (! mutex_trylock(&buttons_lock) )
            return -EBUSY;
    }
    else
    {
        mutex_lock_interruptible(&buttons_lock);
    }
    
    request_irq(IRQ_EINT0,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S2", &pins_desc[0]);
    request_irq(IRQ_EINT2,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S3", &pins_desc[1]);
    request_irq(IRQ_EINT11, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S4", &pins_desc[2]);
    request_irq(IRQ_EINT19, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S5", &pins_desc[3]);

    return 0;
}

static ssize_t keys_drv_read(struct file *filp, char __user *buf, size_t size, loff_t *loft)
{
    if( size != 1)
        return -EINVAL;

    if( (filp->f_flags) & O_NONBLOCK )
    {
        if(ev_press == 0)
            return -EAGAIN;
    }
    else
    {
        wait_event_interruptible(buttons_wq, ev_press != 0);
    }
    
    copy_to_user(buf, &key_val, 1);
    ev_press = 0;
    
    return 1;
}

static unsigned int keys_drv_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    poll_wait(filp, &buttons_wq, wait);

    if(ev_press)
        mask |= POLLIN | POLLRDNORM;
    
    return mask;
}

static int keys_drv_fasync(int fd, struct file *filp, int on)
{
    return fasync_helper(fd, filp, on, &button_fasync);
}

static int keys_drv_release(struct inode *node, struct file *filp)
{
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    mutex_unlock(&buttons_lock);

    return keys_drv_fasync(-1, filp, 0);
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_drv_open,
    .read = keys_drv_read,
    .poll = keys_drv_poll,
    .fasync = keys_drv_fasync,
    .release = keys_drv_release,
};

static int __init keys_drv_init(void)
{
    major = register_chrdev(0, "keys_drv_old", &keys_fops);
    keys_old_cls = class_create(THIS_MODULE, "keys_old_cls");
    device_create(keys_old_cls, NULL, MKDEV(major, 0), NULL, "buttons");

    init_timer(&buttons_timer);
    // buttons_timer.data = (unsigned long)dev;
    buttons_timer.function = buttons_timer_function;
    add_timer(&buttons_timer);
    
    return 0;
}

void keys_drv_exit(void)
{
    del_timer(&buttons_timer);
    device_destroy(keys_old_cls, MKDEV(major, 0));
    class_destroy(keys_old_cls);
    unregister_chrdev(major, "keys_drv_old");
}

module_init(keys_drv_init);
module_exit(keys_drv_exit);

MODULE_LICENSE("GPL");


