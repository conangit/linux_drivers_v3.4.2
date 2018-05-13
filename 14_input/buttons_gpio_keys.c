/*
 * drivers/input/keyboard/gpio-keys.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio_keys.h>

/*
 * Source Insight 搜索任意 struct gpio_keys_platform_data 参照即可
 */

static void jz2440_keys_release(struct device *dev)
{
}

static struct gpio_keys_button jz2440_keys_button[] = {
    {
        .code               = KEY_L,
        .gpio               = S3C2410_GPF(0),
        /* 按键按下是否为低电平 是==1 */
        .active_low         = 1,
        /* cat /proc/interrupts可看见 */
        .desc               = "S2",
        .type               = EV_KEY,
        /* 防抖延时 */
        .debounce_interval  = 50,
        .irq                = IRQ_EINT0,
    },
    {
        .code               = KEY_S,
        .gpio               = S3C2410_GPF(2),
        .active_low         = 1,
        .desc               = "S3",
        .type               = EV_KEY,
        .debounce_interval  = 50,
        .irq                = IRQ_EINT2,
    },
    {
        .code               = KEY_ENTER,
        .gpio               = S3C2410_GPG(3),
        .active_low         = 1,
        .desc               = "S4",
        .type               = EV_KEY,
        .debounce_interval  = 50,
        .irq                = IRQ_EINT11,
    },
    {
        .code               = KEY_ESC,
        .gpio               = S3C2410_GPG(11),
        .active_low         = 1,
        .desc               = "S5",
        .type               = EV_KEY,
        .debounce_interval  = 50,
        .irq                = IRQ_EINT19,
    },
};

static struct gpio_keys_platform_data jz2440_keys_data = {
    .buttons        = jz2440_keys_button,
    .nbuttons       = ARRAY_SIZE(jz2440_keys_button),
    /* 按键是否可重复 */
    .rep = 1,
    /* /sys/devices/platform/gpio-keys/input/input%d/name */
    // 否则为"gpio-keys"
    .name = "jz2440_keys",
};

static struct platform_device jz2440_keys_dev = {
    .name           = "gpio-keys",
    .id             = -1,
    .dev            = {
        .platform_data  = &jz2440_keys_data,
        .release        = jz2440_keys_release,
    }
};


static int __init dev_init(void)
{
    return platform_device_register(&jz2440_keys_dev);
}

static void __exit dev_exit(void)
{
    platform_device_unregister(&jz2440_keys_dev);
}


module_init(dev_init);
module_exit(dev_exit);

MODULE_DESCRIPTION("jz2440 gpio_keys devices");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");




