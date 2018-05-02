#ifndef _KEY_DRV_H
#define _KEY_DRV_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/device.h>


#define KEY_DEBUG
#ifdef KEY_DEBUG
#define debug_print(format,...) printk(KERN_DEBUG "[line:%d]" format,  __LINE__, ##__VA_ARGS__)
#else
#define debug_print(format,...) do{}while(0)
#endif


struct key_plat_data {
    const char *name;
    unsigned int index;
};

struct key_dev {
    int irq;
    struct cdev cdev;
    struct platform_device pdev;
};

#endif
