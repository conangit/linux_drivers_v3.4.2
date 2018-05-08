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
#include <linux/slab.h>
#include <linux/gfp.h>

#include "keys_drv.h"

static struct key_platform_data keys_data[] = {
    {.name = "KEY1", .index = 1},
    {.name = "KEY2", .index = 2},
    {.name = "KEY3", .index = 3},
    {.name = "KEY4", .index = 4}
};

static struct resource keys_res[] = {
    [0] = {
        .start = IRQ_EINT0,
        .end = IRQ_EINT0,
        .flags = IORESOURCE_IRQ
    },
    
    [1] = {
        .start = IRQ_EINT2,
        .end = IRQ_EINT2,
        .flags = IORESOURCE_IRQ
    },

    [2] = {
        .start = IRQ_EINT11,
        .end = IRQ_EINT11,
        .flags = IORESOURCE_IRQ
    },

    [3] = {
        .start = IRQ_EINT19,
        .end = IRQ_EINT19,
        .flags = IORESOURCE_IRQ
    }
};

static void keys_release(struct device *dev)
{
    dbg("%s(): %s\n", __func__, dev_name(dev));
}

static struct platform_device jz2440_keys = {
    .name = "jz2440_keys",
    .id = -1,
    .resource = keys_res,
    .num_resources = ARRAY_SIZE(keys_res),
    .dev = {
        .platform_data = keys_data,
        .release = keys_release,
    },
};

static struct platform_device * jz2440_keys_array[] =
{
    &jz2440_keys,
};

static int __init dev_init(void)
{
     return platform_add_devices(jz2440_keys_array, ARRAY_SIZE(jz2440_keys_array));
}

static void __exit dev_exit(void)
{
    platform_device_unregister(jz2440_keys_array[0]);
}


module_init(dev_init);
module_exit(dev_exit);

MODULE_DESCRIPTION("platform keys devices");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


