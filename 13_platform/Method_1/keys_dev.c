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

static struct resource key1_res[] = {
    [0] = {
        .start = IRQ_EINT0,
        .end = IRQ_EINT0,
        .flags = IORESOURCE_IRQ
    }
};

static struct resource key2_res[] = {
    [0] = {
        .start = IRQ_EINT2,
        .end = IRQ_EINT2,
        .flags = IORESOURCE_IRQ
    }
};

static struct resource key3_res[] = {
    [0] = {
        .start = IRQ_EINT11,
        .end = IRQ_EINT11,
        .flags = IORESOURCE_IRQ
    }
};

static struct resource key4_res[] = {
    [0] = {
        .start = IRQ_EINT19,
        .end = IRQ_EINT19,
        .flags = IORESOURCE_IRQ
    }
};

static void keys_release(struct device *dev)
{
    dbg("%s(): %s\n", __func__, dev_name(dev));
}

static struct platform_device smdk_key1 = {
    .name = "jz2440_key1",
    .id = 1,
    .resource = key1_res,
    .num_resources = ARRAY_SIZE(key1_res),
    .dev = {
        .platform_data = &keys_data[0],
        .release = keys_release,
    },
};

static struct platform_device smdk_key2 = {
    .name = "jz2440_key2",
    .id = 2,
    .resource = key2_res,
    .num_resources = ARRAY_SIZE(key2_res),
    .dev = {
        .platform_data = &keys_data[1],
        .release = keys_release,
    },
};

static struct platform_device smdk_key3 = {
    .name = "jz2440_key3",
    .id = 3,
    .resource = key3_res,
    .num_resources = ARRAY_SIZE(key3_res),
    .dev = {
        .platform_data = &keys_data[2],
        .release = keys_release,
    },
};

static struct platform_device smdk_key4 = {
    .name = "jz2440_keys",
    .id = 4,
    .resource = key4_res,
    .num_resources = ARRAY_SIZE(key4_res),
    .dev = {
        .platform_data = &keys_data[3],
        .release = keys_release,
    },
};

static struct platform_device * smdk_keys[] =
{
    &smdk_key1,
    &smdk_key2,
    &smdk_key3,
    &smdk_key4,
};

static int __init dev_init(void)
{
     return platform_add_devices(smdk_keys, ARRAY_SIZE(smdk_keys));
}

static void __exit dev_exit(void)
{
    int i;

    for (i = 3; i >= 0; i--)
        platform_device_unregister(smdk_keys[i]);
}


module_init(dev_init);
module_exit(dev_exit);

MODULE_DESCRIPTION("platform keys devices");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


