#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/tty_flip.h>
#include <linux/tty_ldisc.h>
#include <linux/gfp.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>



#define TINY_TTY_NR 3
#define TINY_TTY_MAJOR 0

struct tiny_serial {
    /* basic */
    struct mutex m_lock;
    struct tty_port port;
    struct timer_list timer;

    /* for tiocmget and tiocmset functions */
    int msr;    /* MSR shadow */
    int mcr;    /* MCR shadow */
};

static struct tiny_serial *tinys;

static void tiny_timer_function(unsigned long data)
{
    struct tiny_serial *tiny = (struct tiny_serial *)data;
    char buf[] = "QWERTYUIOP";
    int size = ARRAY_SIZE(buf);

    if (!tiny)
        return;

    tty_insert_flip_string(tiny->port.tty, buf, size);

    tty_flip_buffer_push(tiny->port.tty);

    mod_timer(&tiny->timer, jiffies + HZ * 2);
}

static int tiny_open(struct tty_struct * tty, struct file * filp)
{
    struct tiny_serial *tiny;
    int status;

    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);

    tty->driver_data = NULL;
    
    tiny = &tinys[tty->index];

    status = tty_port_open(&tiny->port, tty, filp);
    if (!status)
    {
        tty->driver_data = tiny;
    }

    return status;
}

static void tiny_close(struct tty_struct * tty, struct file * filp)
{
    struct tiny_serial *tiny = tty->driver_data;

    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);

    if (tiny)
        tty_port_close(&tiny->port, tty, filp);
}

static int tiny_write(struct tty_struct * tty, const unsigned char *buf, int count)
{
    struct tiny_serial *tiny = tty->driver_data;
    unsigned long flags;
    int i;
    int retval = -EINVAL;

    // dump_stack();

    if (!tiny)
        return -ENODEV;

    mutex_lock(&tiny->m_lock);

    spin_lock_irqsave(&tiny->port.lock, flags);
    if (tiny->port.count == 0) // port was not opened!
    {
        spin_unlock_irqrestore(&tiny->port.lock, flags);
        goto exit;
    }
    spin_unlock_irqrestore(&tiny->port.lock, flags);

    /* 模拟从用户层收到的数据写到硬件 */
    printk(KERN_DEBUG "%s()\n", __func__);
    for (i = 0; i < count; i++)
        printk(KERN_ERR "%c ", buf[i]);
    retval = count;

exit:
    mutex_unlock(&tiny->m_lock);
    return retval;
}

static int tiny_write_room(struct tty_struct *tty)
{
    struct tiny_serial *tiny = tty->driver_data;
    unsigned long flags;
    int room = -EINVAL;

    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);


    if (!tiny)
        return -ENODEV;

    mutex_lock(&tiny->m_lock);

    spin_lock_irqsave(&tiny->port.lock, flags);
    if (tiny->port.count == 0) // port was not opened!
    {
        spin_unlock_irqrestore(&tiny->port.lock, flags);
        goto exit;
    }
    spin_unlock_irqrestore(&tiny->port.lock, flags);

    room = 255;

exit:
    mutex_unlock(&tiny->m_lock);
    return room;
}

/* 参照 /net/irda/ircomm/ircomm_tty_ioctl */
#define RELEVANT_IFLAG(iflag) (iflag & (IGNBRK|BRKINT|IGNPAR|PARMRK|INPCK))

static void tiny_set_termios(struct tty_struct *tty, struct ktermios * old)
{
    unsigned int c_cflag = tty->termios->c_cflag;

    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);

    if (old)
    {
        if ((old->c_cflag == c_cflag) && 
            (RELEVANT_IFLAG(c_cflag) == RELEVANT_IFLAG(old->c_cflag)))
        {
            printk(KERN_INFO " - nothing no to change\n");
            return;
        }
    }

    /* get the byte size */
    switch (c_cflag & CSIZE) {
        case CS5:
            printk(KERN_INFO " - date bit = 5\n");
            break;
        case CS6:
            printk(KERN_INFO " - date bit = 6\n");
            break;
        case CS7:
            printk(KERN_INFO " - date bit = 7\n");
            break;
        default:
        case CS8:
            printk(KERN_INFO " - date bit = 8\n");
            break;
    }

    /* determine the parity */
    if (c_cflag & PARENB)
    {
        if (c_cflag & PARODD)
            printk(KERN_INFO " - parity = odd\n");
        else
            printk(KERN_INFO " - parity = even\n");
    }
    else
    {
        printk(KERN_INFO " - parity = none\n");
    }

    /* figure out the stop bits requested */
    if (c_cflag & CSTOPB)
        printk(KERN_INFO " - stop bit = 2\n");
    else
        printk(KERN_INFO " - stop bit = 1\n");

    /* figure out the hardware flow control settings */
    if (c_cflag & CRTSCTS)
        printk(KERN_INFO " - CRTS/CTS is enabled\n");
    else
        printk(KERN_INFO " - CRTS/CTS is disabled\n");

    /* determine software flow control */
    /* if we are implementing XON/XOFF, set the start and stop character in the device */
    if (I_IXOFF(tty) | I_IXON(tty))
    {
        unsigned char start_ch = START_CHAR(tty);
        unsigned char stop_ch  = STOP_CHAR(tty);

        /* if we are implementing INBOUND XON/XOFF */
        if (I_IXOFF(tty))
            printk(KERN_INFO " - INBOUND XON/XOFF is enabled, "
                "XON = %2x, XOFF = %2x\n", start_ch, stop_ch);
        else
            printk(KERN_INFO " - INBOUND XON/XOFF is disabled");

        /* if we are implementing OUTBOUND XON/XOFF */
        if (I_IXON(tty))
            printk(KERN_INFO " - OUTBOUND XON/XOFF is enabled, "
                "XON = %2x, XOFF = %2x\n", start_ch, stop_ch);
        else
            printk(KERN_INFO " - OUTBOUND XON/XOFF is disabled");
    }

    /* get the baud rate wanted */
    printk(KERN_INFO " - baud rate = %d\n", tty_get_baud_rate(tty));
}

/* Our fake UART values */
#define MCR_DTR     0x01
#define MCR_RTS     0x02
#define MCR_LOOP    0x04
#define MSR_CTS     0x08
#define MSR_CD      0x10
#define MSR_RI      0x20
#define MSR_DSR     0x40

static int tiny_tiocmget(struct tty_struct *tty)
{
    struct tiny_serial *tiny = tty->driver_data;
    int msr = tiny->msr;
    int mcr = tiny->mcr;
    int result;

    printk(KERN_INFO "%s()\n", __func__);

    result = ((mcr & MCR_DTR)  ? TIOCM_DTR  : 0) |  /* DTR is set */
             ((mcr & MCR_RTS)  ? TIOCM_RTS  : 0) |  /* RTS is set */
             ((mcr & MCR_LOOP) ? TIOCM_LOOP : 0) |  /* LOOP is set */
             ((msr & MSR_CTS)  ? TIOCM_CTS  : 0) |  /* CTS is set */
             ((msr & MSR_CD)   ? TIOCM_CAR  : 0) |  /* Carrier detect is set*/
             ((msr & MSR_RI)   ? TIOCM_RI   : 0) |  /* Ring Indicator is set */
             ((msr & MSR_DSR)  ? TIOCM_DSR  : 0);   /* DSR is set */

    return result;
}

static int tiny_tiocmset(struct tty_struct *tty, unsigned int set, unsigned int clear)
{
    struct tiny_serial *tiny = tty->driver_data;
    int mcr = tiny->mcr;

    printk(KERN_INFO "%s()\n", __func__);

    if (set & TIOCM_RTS)
        mcr |= MCR_RTS;
    if (set & TIOCM_DTR)
        mcr |= MCR_RTS;

    if (clear & TIOCM_RTS)
        mcr &= ~MCR_RTS;
    if (clear & TIOCM_DTR)
        mcr &= ~MCR_RTS;

    tiny->mcr = mcr;
    return 0;
}

static int tiny_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);
    
    switch (cmd) {
    case TIOCSERGETLSR :
        printk(KERN_ERR "TIOCSERGETLSR\n");
        return 0;
    case TIOCMIWAIT :
        printk(KERN_ERR "TIOCMIWAIT\n");
        return 0;
    case TIOCGICOUNT :
        printk(KERN_ERR "TIOCGICOUNT\n");
        return 0;
    }

    return -ENOIOCTLCMD;
}

static int tiny_activate(struct tty_port *tport, struct tty_struct *tty)
{
    // struct tiny_serial *tiny = tty->driver_data;
    struct tiny_serial *tiny = container_of(tport, struct tiny_serial, port);
    
    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);

    if (!tiny)
        return -ENODEV;

    /* 用定时器模拟硬件每2秒发送数据到用户层 */
    init_timer(&tiny->timer);
    tiny->timer.function = tiny_timer_function;
    tiny->timer.data = (unsigned long)tiny;
    tiny->timer.expires = jiffies + HZ * 2; //2s
    add_timer(&tiny->timer);
    
    return 0;
}

static void tiny_shutdown(struct tty_port *tport)
{
    struct tiny_serial *tiny = container_of(tport, struct tiny_serial, port);

    // dump_stack();
    printk(KERN_INFO "%s()\n", __func__);

    if (!tiny)
        return;

    del_timer(&tiny->timer);
}

static const struct tty_port_operations port_ops = {
    .activate = tiny_activate,
    .shutdown = tiny_shutdown,
};

static int tiny_proc_show(struct seq_file *seq, void *v)
{
    int i;

    for (i = 0; i < TINY_TTY_NR; i++)
    {
        if (&tinys[i] == NULL)
            continue;
        seq_printf(seq, "%d: %s()\n", i, __func__);
    }
    return 0;
}

static int tiny_proc_open(struct inode *inode, struct file *filp)
{
    // single_open( , , void *data) data参数将传给filp->private_data
    return single_open(filp, tiny_proc_show, NULL);
}

static ssize_t tiny_proc_write(struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    printk("%s()\n", __func__);
    return size;
}

static const struct file_operations tiny_proc_fops = {
    .owner = THIS_MODULE,
    .open = tiny_proc_open,
    .write = tiny_proc_write,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static struct tty_operations serial_ops = {
    .open = tiny_open,
    .close = tiny_close,
    .write = tiny_write,
    .write_room = tiny_write_room,
    .set_termios = tiny_set_termios,
    .tiocmget = tiny_tiocmget,
    .tiocmset = tiny_tiocmset,
    .ioctl = tiny_ioctl,
    .proc_fops = &tiny_proc_fops,
};

static struct tty_driver *tiny_tty_driver;

static int __init tiny_init(void)
{
    int retval;
    int i;

    /* alloc_tty_driver()调用会将TINY_TTY_MINORS放入driver->num */
    tiny_tty_driver = alloc_tty_driver(TINY_TTY_NR);
    if (!tiny_tty_driver)
        return -ENOMEM;

    tiny_tty_driver->owner = THIS_MODULE;
    tiny_tty_driver->name = "ttyPL";             // "/dev/ttyPL%d"
    tiny_tty_driver->driver_name = "pl_uart_tty";
    tiny_tty_driver->major = TINY_TTY_MAJOR;
    tiny_tty_driver->type = TTY_DRIVER_TYPE_SERIAL;
    tiny_tty_driver->subtype = SERIAL_TYPE_NORMAL;
    tiny_tty_driver->flags = TTY_DRIVER_REAL_RAW | TTY_DRIVER_DYNAMIC_DEV;
    tiny_tty_driver->init_termios = tty_std_termios;
    tiny_tty_driver->init_termios.c_cflag = B115200 | CS8 | CREAD | HUPCL | CLOCAL;
    tiny_tty_driver->init_termios.c_ispeed = tiny_tty_driver->init_termios.c_ospeed = 115200;

    tty_set_operations(tiny_tty_driver, &serial_ops);

    retval = tty_register_driver(tiny_tty_driver);
    if (retval)
    {
        printk(KERN_ERR "failed to register tiny tty driver\n");
        goto error0;
    }

    tinys = kmalloc(sizeof(struct tiny_serial) * TINY_TTY_NR, GFP_KERNEL);
    if (!tinys)
    {
        printk(KERN_ERR "failed to alloc tiny_serial\n");
        retval = -ENOMEM;
        goto error1;
    }

    for (i = 0; i < TINY_TTY_NR; i++)
    {
        mutex_init(&tinys[i].m_lock);
        tty_port_init(&tinys[i].port);
        tinys[i].port.ops = &port_ops;
    }

    /* tiny_tty_driver->flags = TTY_DRIVER_DYNAMIC_DEV; */
    /* 当设置此标志位时, 必须显示调用tty_register_device() */
    /* 否则tty_register_driver()将根据driver->num自动注册devices */
    for (i = 0; i < TINY_TTY_NR; i++) {
        tty_register_device(tiny_tty_driver, i, NULL);
    }

    printk("LINUX_VERSION_CODE = %u, KERNEL_VERSION(3, 4, 2) = %u\n", LINUX_VERSION_CODE, KERNEL_VERSION(3, 4, 2));

    return retval;

error1:
    tty_unregister_driver(tiny_tty_driver);
error0:
    put_tty_driver(tiny_tty_driver);
    return retval;
}

static void __exit tiny_exit(void)
{
    int i;

    for (i = 0; i < TINY_TTY_NR; i++)
        tty_unregister_device(tiny_tty_driver, i);

    if (tinys)
        kfree(tinys);
    tinys = NULL;

    if (tiny_tty_driver)
    {
        tty_unregister_driver(tiny_tty_driver);
        put_tty_driver(tiny_tty_driver);
    }
    tiny_tty_driver = NULL;
}


module_init(tiny_init);
module_exit(tiny_exit);

MODULE_DESCRIPTION("tiny tty driver for Linux v3.4.2");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


