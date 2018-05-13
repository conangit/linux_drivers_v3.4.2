#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/serial_core.h>
#include <linux/tty_flip.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>



/* Documentation/serial/driver */

#define UART_NR 10

struct uart_dev {
    struct uart_port port;
    struct timer_list timer;
};

static struct uart_dev *devs[UART_NR];


static unsigned int tiny_tx_empty(struct uart_port *uport)
{
    return 0;
}

static void tiny_set_mctrl(struct uart_port *uport, unsigned int mctrl)
{

}

static unsigned int tiny_get_mctrl(struct uart_port *uport)
{
    return 0;
}

static void tiny_timer_func(unsigned long data)
{
    struct uart_dev *dev = (struct uart_dev *)data;
    struct uart_port *uport = &dev->port;
    struct tty_port *tport;
    char buf[] = "QWERTYUIOP";
    int size = ARRAY_SIZE(buf);
    int i;

    if (!uport->state)
        return;
    tport = &uport->state->port;

    for (i = 0; i < size; i++)
        // tty_insert_flip_char(tport, buf[i], TTY_NORMAL);
        tty_insert_flip_char(tport->tty, buf[i], TTY_NORMAL);

    // tty_flip_buffer_push(tport);
    tty_flip_buffer_push(tport->tty);

    mod_timer(&dev->timer, jiffies + HZ * 5);
}

static int tiny_startup(struct uart_port *uport)
{
    struct uart_dev *dev = (struct uart_dev *)uport->private_data;

    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    init_timer(&dev->timer);
    dev->timer.data = (unsigned long)dev;
    dev->timer.function = tiny_timer_func;
    dev->timer.expires = jiffies + HZ * 5;
    add_timer(&dev->timer);
    return 0;
}

static void tiny_shutdown(struct uart_port *uport)
{
    struct uart_dev *dev = (struct uart_dev *)uport->private_data;

    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);
    
    del_timer(&dev->timer);
}

static void tiny_set_termios(struct uart_port *uport, struct ktermios *new,
                struct ktermios *old)
{
    unsigned int c_cflag = new->c_cflag;
    unsigned int baud_rate, quot;
    unsigned long flags;

    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);
    
    spin_lock_irqsave(&uport->lock, flags);

    /* get the byte size */
    switch (c_cflag & CSIZE) {
        case CS5:
            printk(KERN_INFO " - date bit = 5\n");
            mdelay(1);
            break;
        case CS6:
            printk(KERN_INFO " - date bit = 6\n");
            mdelay(1);
            break;
        case CS7:
            printk(KERN_INFO " - date bit = 7\n");
            mdelay(1);
            break;
        default:
        case CS8:
            printk(KERN_INFO " - date bit = 8\n");
            mdelay(1);
            break;
    }

    /* determine the parity */
    if (c_cflag & PARENB)
    {
        if (c_cflag & PARODD) {
            printk(KERN_INFO " - parity = odd\n");
            mdelay(1);
        }
        else {
            printk(KERN_INFO " - parity = even\n");
            mdelay(1);
        }
    }
    else
    {
        printk(KERN_INFO " - parity = none\n");
        mdelay(1);
    }

    /* figure out the stop bits requested */
    if (c_cflag & CSTOPB) {
        printk(KERN_INFO " - stop bit = 2\n");
        mdelay(1);
    }
    else {
        printk(KERN_INFO " - stop bit = 1\n");
        mdelay(1);
    }

    baud_rate = uart_get_baud_rate(uport, new, old, 0, uport->uartclk/16);
    quot = uart_get_divisor(uport, baud_rate);
    printk("%s() baud_rate = %u, quot = %u\n", __func__, baud_rate, quot);

    spin_unlock_irqrestore(&uport->lock, flags);
}

static void tiny_stop_tx(struct uart_port *uport)
{
    unsigned long flags;
    
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    spin_lock_irqsave(&uport->lock, flags);
    uport->fifosize = 0;
    spin_unlock_irqrestore(&uport->lock, flags);
}

static void tiny_start_tx(struct uart_port *uport)
{
    struct circ_buf *xmit = &uport->state->xmit;
    int count;
    
    if (uport->x_char)
    {
        printk("x_char %c\n", uport->x_char);
        uport->icount.tx++;
        uport->x_char = 0;
        return;
    }

    if (uart_circ_empty(xmit) || uart_tx_stopped(uport))
    {
        tiny_stop_tx(uport);
        return;
    }

    count = uport->fifosize;
    printk("do write:\n");
    mdelay(1);
    do {
        printk("%c", xmit->buf[xmit->tail]);
        xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
        uport->icount.tx++;
        if (uart_circ_empty(xmit))
            break;
    }while(--count > 0);

    if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
        uart_write_wakeup(uport);

    if (uart_circ_empty(xmit))
        tiny_stop_tx(uport);
    return;
}

static void tiny_stop_rx(struct uart_port *uport)
{
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);
    // disable rx_irq
}

static const char* tiny_type(struct uart_port *uport)
{
    //操作proc时被调用 
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    return "tiny_type";
}

static int tiny_request_port(struct uart_port *uport)
{
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    // for ioremap retunrn 0 or -EBUSY

    return 0;
}

static void tiny_config_port(struct uart_port *uport, int flags)
{
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    // 必须再次指定uport type
    if (tiny_request_port(uport) == 0 )
        uport->type = PORT_S3C2410;
}

static void tiny_release_port(struct uart_port *uport)
{
    // 卸载驱动时被调用
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    // for iounmap
}

static int tiny_verify_port(struct uart_port *uport, struct serial_struct *ser)
{
    printk(KERN_INFO "%s()\n", __func__);
    mdelay(1);

    if (uport->type == PORT_S3C2410)
        return 0;
    else
        return -EINVAL;
}

static struct uart_ops tiny_uart_ops = {
    // 必须定义下三函数
    .tx_empty = tiny_tx_empty,
    .set_mctrl = tiny_set_mctrl,
    .get_mctrl = tiny_get_mctrl,
    // 发送数据相关
    .stop_tx = tiny_stop_tx,
    .start_tx = tiny_start_tx,
    // .throttle = tiny_throttle,
    // .unthrottle= tiny_unthrottle,
    // .send_xchar = tiny_send_xchar,
    .stop_rx = tiny_stop_rx,
    // .enable_ms = tiny_enable_ms,
    // .break_ctl = tiny_break_ctl,
    .startup = tiny_startup,
    .shutdown = tiny_shutdown,
    // .flush_buffer = tiny_flush_buffer,
    .set_termios = tiny_set_termios,
    // .set_ldisc = tiny_set_ldisc,
    // .pm = tiny_pm,
    .type = tiny_type,
    .release_port = tiny_release_port,
    .request_port = tiny_request_port,
    .config_port = tiny_config_port,
    .verify_port = tiny_verify_port,
};

static struct uart_driver tiny_uart_drv = {
    .owner       = THIS_MODULE,
    .driver_name = "tiny_uart",
    .dev_name    = "ttyPL",
    .major       = 0,
    .minor       = 0,
    .nr          = UART_NR,
    .cons        = NULL,
};


static int __init tiny_uart_init(void)
{
    int ret;
    struct uart_dev *dev;
    unsigned int i;
    dma_addr_t cma_handle;

    ret = uart_register_driver(&tiny_uart_drv);
    if (ret)
    {
        printk(KERN_ERR "Error: register uart driver failed\n");
        return ret;
    }

    for (i = 0; i < UART_NR; i++)
    {
        dev = kmalloc(sizeof(struct uart_dev), GFP_KERNEL);
        if (!dev)
        {
            printk(KERN_ERR "Error: no memory for uart devices-%d\n", i);
            ret = -ENOMEM;
            goto error0;
        }

        dev->port.type      = PORT_S3C2410;
        dev->port.iotype    = UPIO_MEM;
        dev->port.flags     = UPF_BOOT_AUTOCONF; //设置此标志位 才会调用config_port()
        dev->port.uartclk   = 50000000;
        dev->port.fifosize  = 16;
        dev->port.line      = i;
        dev->port.mapbase   = virt_to_phys(dma_zalloc_coherent(NULL, PAGE_SIZE, &cma_handle, GFP_KERNEL));
        // dev->port.irq       = 16 + i;
        // dev->port.irqflags  = ...;
        dev->port.ops       = &tiny_uart_ops;
        spin_lock_init(&dev->port.lock);
        
        ret = uart_add_one_port(&tiny_uart_drv, &dev->port);
        if (ret)
        {
            printk(KERN_ERR "Error: add uart port-%d failed\n", i);
            goto error0;
        }

        dev->port.private_data = dev;
        devs[i] = dev;
    }

    printk("Register tiny uart driver\n");

    return ret;

error0:
    for(i = i - 1; i >=0 ; i--)
    {
        struct uart_dev *dev = devs[i];
        uart_remove_one_port(&tiny_uart_drv, &dev->port);
        kfree(dev);
        dev = NULL;
    }
    uart_unregister_driver(&tiny_uart_drv);
    return ret;
}


static void __exit tiny_uart_exit(void)
{
    int i;
    struct uart_dev *dev;

    for(i = UART_NR - 1; i >=0; i--)
    {
        dev = devs[i];
        uart_remove_one_port(&tiny_uart_drv, &dev->port);
        kfree(dev);
        dev = NULL;
    }
    uart_unregister_driver(&tiny_uart_drv);
    printk("Remove tiny uart driver\n");
}


module_init(tiny_uart_init);
module_exit(tiny_uart_exit);

MODULE_DESCRIPTION("tiny uart driver for Linux v4.9.x");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


