/*
 * drivers/input/keyboard/gpio-keys
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/irqreturn.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/platform_device.h>

#include "buttons_drv.h"


struct key_dev {
    int irq;
    int gpio;
    struct input_dev *btn_input_dev;
    struct btn_platform_data *priv_data;
    struct work_struct work;
    struct timer_list timer;
    int index;
};

static volatile int probe_index = 0;


static irqreturn_t btn_irq_handler(int irq, void *dev_id)
{
    struct key_dev *priv = (struct key_dev *)dev_id;
    schedule_work(&priv->work);
    return IRQ_HANDLED;
}

static void btn_work_func(struct work_struct *work)
{
    struct key_dev *priv = container_of(work, struct key_dev, work);
    priv->timer.data = (unsigned long)priv;
    mod_timer(&priv->timer, jiffies + HZ / 20);
}

static void btn_timer_func(unsigned long data)
{
    struct key_dev *priv = (struct key_dev *)data;
    unsigned int pinval = s3c2410_gpio_getpin(priv->gpio);

    if(pinval)
        // input_event(priv->btn_input_dev, EV_KEY, priv->priv_data->code, 0);     // 弹起
        input_report_key(priv->btn_input_dev, priv->priv_data->code, 0);
    else
        input_event(priv->btn_input_dev, EV_KEY, priv->priv_data->code, 1);     // 按下

    input_sync(priv->btn_input_dev);
}

static int btn_probe(struct platform_device *pdev)
{
    struct key_dev *priv;
    struct resource *r;
    int ret = 0;

    priv = devm_kzalloc(&pdev->dev, sizeof(struct key_dev), GFP_KERNEL);
    if (!priv)
    {
        dev_err(&pdev->dev, "Cannot alloc memory for device\n");
        return -ENOMEM;
    }

    priv->btn_input_dev = input_allocate_device();
    if (!priv->btn_input_dev)
    {
        dev_err(&pdev->dev, "Cannot alloc memory for input device\n");
        ret = -ENOMEM;
        goto error0;
    }

    set_bit(EV_KEY, priv->btn_input_dev->evbit);
    set_bit(EV_REP, priv->btn_input_dev->evbit);

    // device resources
    // irq
    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0)
    {
        dev_err(&pdev->dev, "Cannot get device irq, %d\n", priv->irq);
        goto error1;
    }
    dbg("device irq: %d\n", priv->irq);

    // gpio
    r = platform_get_resource(pdev, IORESOURCE_IO, 0);
    priv->gpio = r->start;
    dbg("device gpio: %d\n", priv->gpio);

    // device platform_data
    priv->priv_data = (struct btn_platform_data *)dev_get_platdata(&pdev->dev);
    dbg("name: %s, code: %d\n", priv->priv_data->name, priv->priv_data->code);
    set_bit(priv->priv_data->code, priv->btn_input_dev->keybit);

    ret = input_register_device(priv->btn_input_dev);
    if (ret)
    {
        dev_err(&pdev->dev, "Unable to register buttons input device\n");
        goto error1;
    }

    INIT_WORK(&priv->work, btn_work_func);

    // Lastly request irq
    init_timer(&priv->timer);
    priv->timer.function = btn_timer_func;

    ret = request_irq(priv->irq, btn_irq_handler, IRQ_TYPE_EDGE_BOTH, priv->priv_data->name, priv);
    // error
    // ret = request_irq(priv->irq, btn_irq_handler, IRQ_TYPE_EDGE_FALLING, priv->priv_data->name, priv);
    if (ret)
    {
        dev_err(&pdev->dev, "Unable to request irq for input device\n");
        goto error2;
    }

    priv->index = probe_index;
    probe_index++;
    platform_set_drvdata(pdev, priv);

    return ret;

error2:
    del_timer(&priv->timer);
    input_unregister_device(priv->btn_input_dev);
error1:
    input_free_device(priv->btn_input_dev);
error0:
    if (priv)
        devm_kfree(&pdev->dev, priv);
    return ret;
}

static int btn_remove(struct platform_device *pdev)
{
    struct key_dev *priv = platform_get_drvdata(pdev);

    probe_index--;
    if (priv)
    {
        free_irq(priv->irq, priv);
        del_timer(&priv->timer);
        input_unregister_device(priv->btn_input_dev);
        input_free_device(priv->btn_input_dev);
        devm_kfree(&pdev->dev, priv);
    }
    priv = NULL;

    return 0;
}


static struct platform_driver jz2440_btn_platform_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name  = "jz2440_buttons",
    },
    .probe  = btn_probe,
    .remove = btn_remove,
};

module_platform_driver(jz2440_btn_platform_driver);


MODULE_DESCRIPTION("jz2440 gpio_keys driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


