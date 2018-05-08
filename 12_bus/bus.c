#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/string.h>


static int bus_match(struct device *dev, struct device_driver *drv)
{
    return (strcmp(dev->kobj.name, drv->name) == 0);
}

#if 0
static int bus_probe(struct device *dev)
{
    return 0;
}

static int bus_remove(struct device *dev)
{
    return 0;
}
#endif

/* 
 * Warinng: 
 * Driver 'lihong' needs updating - please use bus_type methods
 * drivers/base/driver.c :
 * 	if ((drv->bus->probe && drv->probe) ||
        (drv->bus->remove && drv->remove) ||
        (drv->bus->shutdown && drv->shutdown))
        printk(KERN_WARNING "Driver '%s' needs updating - please use "
        "bus_type methods\n", drv->name);
 */
 
struct bus_type virtual_bus = {
    .name = "virtual_bus",
    .match = bus_match,
    // .probe = bus_probe,
    // .remove = bus_remove,
};
EXPORT_SYMBOL(virtual_bus);

static int __init virtual_bus_init(void)
{
    return bus_register(&virtual_bus);
}

static void __exit virtual_bus_exit(void)
{
    bus_unregister(&virtual_bus);
}


module_init(virtual_bus_init);
module_exit(virtual_bus_exit);


MODULE_DESCRIPTION("bus type");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");

