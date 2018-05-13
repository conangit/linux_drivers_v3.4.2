/*
 * drivers/input/keyboard/gpio-keys
 */

#include <linux/module.h>
#include <linux/version.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/irq.h>

#include <asm/gpio.h>

#include <linux/fcntl.h>
#include <linux/timer.h>

#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/gfp.h>

static struct input_dev *buttons_dev = NULL;

static struct work_struct *buttons_irq_work = NULL;

static struct timer_list buttons_timer;

struct pin_desc{
    int irq;
    unsigned int pin;
    unsigned int key_val;
    const char* name;
};

struct pin_desc pins_desc[4] = {
    {IRQ_EINT0,  S3C2410_GPF0,  KEY_L,          "S2"},
    {IRQ_EINT2,  S3C2410_GPF2,  KEY_S,          "S3"},
    {IRQ_EINT11, S3C2410_GPG3,  KEY_ENTER,      "S4"},
    {IRQ_EINT19, S3C2410_GPG11, KEY_LEFTSHIFT,  "S5"},
};

struct pin_desc* irq_pd = NULL;

static irq_handler_t buttons_irq(int irq, void *dev_id)
{
    irq_pd = (struct pin_desc*)dev_id;
    
    schedule_work(buttons_irq_work);
    
    return IRQ_RETVAL(IRQ_HANDLED);
}

static void buttons_work_func(struct work_struct *work)
{
    mod_timer(&buttons_timer, jiffies + HZ/100);        //10ms
}


static void buttons_timer_func(unsigned long data)
{
    struct pin_desc* pindesc = irq_pd;

    if(pindesc != NULL)
    {
        unsigned int pinval = s3c2410_gpio_getpin(pindesc->pin);

        if(pinval)
        {
            input_event(buttons_dev, EV_KEY, pindesc->key_val, 0);
            input_sync(buttons_dev);
        }
        else
        {
            input_event(buttons_dev, EV_KEY, pindesc->key_val, 1);
            input_sync(buttons_dev);
        }
    }
}

static int buttons_init(void)
{
    int error = 0;
    int i = 0;
    
    buttons_dev = input_allocate_device();

    if (!buttons_dev)
    {
        return -ENOMEM;
    }
    
    set_bit(EV_KEY, buttons_dev->evbit);
    set_bit(EV_REP, buttons_dev->evbit);

    set_bit(KEY_L, buttons_dev->keybit);
    set_bit(KEY_S, buttons_dev->keybit);
    set_bit(KEY_ENTER, buttons_dev->keybit);
    set_bit(KEY_LEFTSHIFT, buttons_dev->keybit);

    error = input_register_device(buttons_dev);
    if (error)
    {
        printk(KERN_ERR "Unable to register buttons_dev input device\n");
        goto fail;
    }
    
    for(i = 0; i < 4; i++)
    {
        error = request_irq(pins_desc[i].irq, buttons_irq, IRQT_BOTHEDGE, pins_desc[i].name, &pins_desc[i]);
        if(error)
        {
            printk(KERN_ERR "buttons: unable to claim irq %d; error %d\n", pins_desc[i].irq, error);
            goto fail;
        }
    }

    buttons_irq_work = (struct work_struct*)kmalloc(sizeof(struct work_struct), GFP_KERNEL);
    INIT_WORK(buttons_irq_work, buttons_work_func);

    init_timer(&buttons_timer);
    buttons_timer.function = buttons_timer_func;
    add_timer(&buttons_timer);

    return 0;

fail:
    for(i = i -1; i >=0; i--)
    {
        free_irq(pins_desc[i].irq, &pins_desc[i]);
    }

    input_free_device(buttons_dev);

    return error;
}


static void buttons_exit(void)
{
    int i = 0;
    
    input_free_device(buttons_dev);
    
    for(i = 0; i < 4; i++)
    {
        free_irq(pins_desc[i].irq, &pins_desc[i]);
    }
    
    input_unregister_device(buttons_dev);

    kfree(buttons_irq_work);

    del_timer(&buttons_timer);
}



module_init(buttons_init);
module_exit(buttons_exit);

MODULE_LICENSE("GPL");

