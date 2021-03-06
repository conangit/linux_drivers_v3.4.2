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

struct key_dev {
    int irq;
    struct cdev cdev;
    struct key_platform_data *priv_data;
};


static int key_probe(struct platform_device *pdev)
{
    struct key_dev *priv;
    int ret;
    const struct platform_device_id *platid;
    
    dbg("## %s() ##\n", __func__);

    priv = devm_kzalloc(&pdev->dev, sizeof(struct key_dev), GFP_KERNEL);
    if (!priv)
    {
        dev_err(&pdev->dev, "Cannot alloc memory for device\n");
        return -ENOMEM;
    }

    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0)
    {
        dev_err(&pdev->dev, "Cannot get device irq, %d\n", priv->irq);
        ret = priv->irq;
        goto error0;
    }
    dbg("%s irq: %d\n", dev_name(&pdev->dev), priv->irq);

    priv->priv_data = dev_get_platdata(&pdev->dev);
    dbg("name: %s, index: %d\n", priv->priv_data->name, priv->priv_data->index);

    platid = platform_get_device_id(pdev);
    if (platid)
        dbg("id_table, name: %s, version: %lu\n", platid->name, platid->driver_data);

    platform_set_drvdata(pdev, priv);
    
    return 0;

error0:
    if (priv)
        devm_kfree(&pdev->dev, priv);
    return ret;
}

static int key_remove(struct platform_device *pdev)
{
    struct key_dev *priv = platform_get_drvdata(pdev);

    dbg("## %s() ##\n", __func__);
    dbg("%s\n", dev_name(&pdev->dev));

    devm_kfree(&pdev->dev, priv);

    return 0;
}

static const struct platform_device_id keys_id_table[] = {
    {"jz2440_key1", 0x01},
    {"jz2440_key2", 0x02},
    {"jz2440_key3", 0x03},
    {"jz2440_key4", 0x04},
    {}
};

static struct platform_driver key_drv = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = "jz2440_keys",
    },
    .probe    = key_probe,
    .remove   = key_remove,
    .id_table = keys_id_table,
};

module_platform_driver(key_drv);

MODULE_DESCRIPTION("platform keys driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


