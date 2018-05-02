#include "keys_drv.h"

static struct key_plat_data keys_data[] = {
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


static struct platform_device smdk_key1 = {
    // .name = "keys_x",
    .name = "keys_y",
    .id = -1,
    .resource = key1_res,
    .num_resources = ARRAY_SIZE(key1_res),
    .dev = {
        .platform_data = &keys_data[0],
    },
};

static struct platform_device smdk_key2 = {
    // .name = "keys_x",
    .name = "keys_y",
    .id = -1,
    .resource = key2_res,
    .num_resources = ARRAY_SIZE(key2_res),
    .dev = {
        .platform_data = &keys_data[1],
    },
};

static struct platform_device smdk_key3 = {
    // .name = "keys_x",
    .name = "keys_y",
    .id = -1,
    .resource = key3_res,
    .num_resources = ARRAY_SIZE(key3_res),
    .dev = {
        .platform_data = &keys_data[2],
    },
};


static struct platform_device smdk_key4 = {
    // .name = "keys_x",
    .name = "keys_y",
    .id = -1,
    .resource = key4_res,
    .num_resources = ARRAY_SIZE(key4_res),
    .dev = {
        .platform_data = &keys_data[3],
    },
};


static struct platform_device* smdk_keys[] =
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

module_init(dev_init);

MODULE_DESCRIPTION("platform keys devices");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


