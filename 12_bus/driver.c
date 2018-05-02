#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>

extern struct bus_type virtual_bus;

static int drv_probe(struct device * dev)
{
    printk(KERN_INFO "driver found the device that it can handle\n");
    
    return 0;
}

static int drv_remove(struct device *dev)
{
    printk(KERN_INFO "%s\n", __func__);
    return 0;
}

#if 0
static const struct of_device_id virtural_of_match[] = {
    {.compatible = "lihong"},
    { },
};

MODULE_DEVICE_TABLE(of, virtural_of_match);
#endif

struct device_driver virtual_drv = {
    .owner = THIS_MODULE,
    .name = "lihong",
    // .of_match_table = virtural_of_match,
    .bus = &virtual_bus,
    .probe = drv_probe,
};

static int mydrv_init(void)
{
    return driver_register(&virtual_drv);
}

static void mydrv_exit(void)
{
    driver_unregister(&virtual_drv);
}

module_init(mydrv_init);
module_exit(mydrv_exit);

MODULE_DESCRIPTION("virtual driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");

