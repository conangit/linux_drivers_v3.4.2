#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/input.h>
#include "buttons_drv.h"

static struct btn_platform_data btns_data[] = {
    {.name = "S2", .code = KEY_L},          // 38
    {.name = "S3", .code = KEY_S},          // 31
    {.name = "S4", .code = KEY_ENTER},      // 28
    {.name = "S5", .code = KEY_ESC}         // 1
};

static struct resource btn1_res[] = {
    [0] = {
        .start = IRQ_EINT0,
        .end = IRQ_EINT0,
        .flags = IORESOURCE_IRQ     // 16
    },
    [1] = {
        .start = S3C2410_GPF(0),    // 160
        .end = S3C2410_GPF(0),
        .flags = IORESOURCE_IO
    }
};

static struct resource btn2_res[] = {
    [0] = {
        .start = IRQ_EINT2,         // 18
        .end = IRQ_EINT2,
        .flags = IORESOURCE_IRQ
    },
    [1] = {
        .start = S3C2410_GPF(2),    // 162
        .end = S3C2410_GPF(2),
        .flags = IORESOURCE_IO
    }
};

static struct resource btn3_res[] = {
    [0] = {
        .start = IRQ_EINT11,        // 55
        .end = IRQ_EINT11,
        .flags = IORESOURCE_IRQ
    },
    [1] = {
        .start = S3C2410_GPG(3),    // 195
        .end = S3C2410_GPG(3),
        .flags = IORESOURCE_IO
    }
};

static struct resource btn4_res[] = {
    [0] = {
        .start = IRQ_EINT19,        // 63
        .end = IRQ_EINT19,
        .flags = IORESOURCE_IRQ
    },
    [1] = {
        .start = S3C2410_GPG(11),   // 203
        .end = S3C2410_GPG(11),
        .flags = IORESOURCE_IO
    }
};

static void btns_release(struct device *dev)
{
    dbg("%s(): %s\n", __func__, dev_name(dev));
}

static struct platform_device jz2440_btn1 = {
    .name = "jz2440_buttons",
    .id = 2,
    .resource = btn1_res,
    .num_resources = ARRAY_SIZE(btn1_res),
    .dev = {
        .platform_data = &btns_data[0],
        .release = btns_release,
    },
};

static struct platform_device jz2440_btn2 = {
    .name = "jz2440_buttons",
    .id = 3,
    .resource = btn2_res,
    .num_resources = ARRAY_SIZE(btn2_res),
    .dev = {
        .platform_data = &btns_data[1],
        .release = btns_release,
    },
};

static struct platform_device jz2440_btn3 = {
    .name = "jz2440_buttons",
    .id = 4,
    .resource = btn3_res,
    .num_resources = ARRAY_SIZE(btn3_res),
    .dev = {
        .platform_data = &btns_data[2],
        .release = btns_release,
    },
};

static struct platform_device jz2440_btn4 = {
    .name = "jz2440_buttons",
    .id = 5,
    .resource = btn4_res,
    .num_resources = ARRAY_SIZE(btn4_res),
    .dev = {
        .platform_data = &btns_data[3],
        .release = btns_release,
    },
};

static struct platform_device * jz2440_btns[] =
{
    &jz2440_btn1,
    &jz2440_btn2,
    &jz2440_btn3,
    &jz2440_btn4,
};

static int __init dev_init(void)
{
     return platform_add_devices(jz2440_btns, ARRAY_SIZE(jz2440_btns));
}

static void __exit dev_exit(void)
{
    int i;

    for (i = ARRAY_SIZE(jz2440_btns) - 1; i >= 0; i--)
        platform_device_unregister(jz2440_btns[i]);
}


module_init(dev_init);
module_exit(dev_exit);

MODULE_DESCRIPTION("jz2440 buttons devices");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");



