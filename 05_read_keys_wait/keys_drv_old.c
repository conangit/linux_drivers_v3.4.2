#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include <asm/io.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/device.h>


int major = 0;

static struct class *keys_class_old;

struct pin_desc{
    unsigned int pin;
    unsigned char key_val;
};

unsigned char key_val = 0;

static int ev_press = 0;
static DECLARE_WAIT_QUEUE_HEAD(buttons_wq);

/*
 * 键值: 按下0x01,0x02,0x03,0x04
 * 键值: 松开0x81,0x82,0x83,0x84
 */
struct pin_desc pins_desc[4] = {
    {S3C2410_GPF(0),   0x01},
    {S3C2410_GPF(2),   0x02},
    {S3C2410_GPG(3),   0x03},
    {S3C2410_GPG(11),  0x04},
};

static irq_handler_t buttons_irq(int irq, void *dev_id)
{
    struct pin_desc* pindesc = (struct pin_desc*)dev_id;

    unsigned int pinval = s3c2410_gpio_getpin(pindesc->pin);

    if(pinval)  //松开
    {
        key_val = 0x80 | (pindesc->key_val);

    }
    else       //按下 
    {
        key_val = pindesc->key_val;
    }

    ev_press = 1;
    wake_up_interruptible(&buttons_wq);
    //wake_up(&buttons_wq);
    
    
    return IRQ_HANDLED;
}

static int keys_drv_open(struct inode *node, struct file *filp)
{
    request_irq(IRQ_EINT0,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S2", &pins_desc[0]);  //16
    request_irq(IRQ_EINT2,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S3", &pins_desc[1]);  //18
    request_irq(IRQ_EINT11, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S4", &pins_desc[2]);  //55
    request_irq(IRQ_EINT19, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S5", &pins_desc[3]);  //63
    ev_press = 0;

    return 0;
}

static ssize_t keys_drv_read(struct file *filp, char __user *buf, size_t size, loff_t *loft)
{
    if( size != 1)
        return -EINVAL;

    //如果没有按键发生，休眠
    //ev_press = 0
    //允许等待该信号的用户空间进程可被中断 如Ctrl + C
    if (wait_event_interruptible(buttons_wq, ev_press != 0))
        return -ERESTARTSYS;
    //不推荐
    //wait_event(buttons_wq, ev_press);
    
    copy_to_user(buf, &key_val, 1);
    
    ev_press = 0;
    
    return 1;
}

int keys_drv_close(struct inode *node, struct file *filp)
{
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);

    return 0;
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_drv_open,
    .read = keys_drv_read,
    .release = keys_drv_close
};

static int keys_drv_init(void)
{
    major = register_chrdev(0, "keys_old_drv", &keys_fops);
    keys_class_old = class_create(THIS_MODULE, "keys_class_old");
    device_create(keys_class_old, NULL, MKDEV(major, 0), NULL, "buttons");

    return 0;
}

static void keys_drv_exit(void)
{
    device_destroy(keys_class_old, MKDEV(major, 0));
    class_destroy(keys_class_old);
    unregister_chrdev(major, "keys_old_drv");
}

module_init(keys_drv_init);
module_exit(keys_drv_exit);

MODULE_LICENSE("GPL");


