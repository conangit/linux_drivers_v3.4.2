#include "keys_drv.h"

static int key_probe(struct platform_device *pdev)
{
    debug_print("%s\n", __func__);
    
    return 0;
}

static int key_remove(struct platform_device *pdev)
{
    debug_print("%s\n", __func__);

    return 0;
}


static const struct of_device_id key_of_match[] = {
    {.compatible = "keys_y"},
    {}
};

MODULE_DEVICE_TABLE(of, key_of_match);

static struct platform_driver key_drv = {
    .probe = key_probe,
    .remove = key_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = "keys_x",
        .of_match_table = key_of_match
    }
};

module_platform_driver(key_drv);

MODULE_DESCRIPTION("platform keys driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


