#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <asm/traps.h>

extern struct bus_type virtual_bus;

void virtual_dev_release(struct device *dev)
{
}


struct device virtual_dev = {
    // .init_name = "lihong",

    // kobj 为结构体
    .kobj = {
        .name = "lihong",
    },

    // of_node 为结构体指针
    
    .bus = &virtual_bus,
    .release = virtual_dev_release,
};

static int __init virtual_dev_init(void)
{
    return device_register(&virtual_dev);
}

static void __exit virtual_dev_exit(void)
{
    device_unregister(&virtual_dev);
}


module_init(virtual_dev_init);
module_exit(virtual_dev_exit);

MODULE_DESCRIPTION("virtual device");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


