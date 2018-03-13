#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/cdev.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/gpio.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>


static int major = 0;
static struct class *keys_old_cls;

struct pin_desc{
    unsigned int pin;
    unsigned int key_val;
};

static int key_val = 0;

static volatile int ev_press = 0;
static DECLARE_WAIT_QUEUE_HEAD(buttons_wq);

static struct fasync_struct *button_fasync;


//信号量
#if 0
//static struct semaphore sem;
static DEFINE_SEMAPHORE(sem);
#endif

//互斥体
#if 0
//static struct mutex lock;
static DEFINE_MUTEX(lock);
#endif

//原子变量
#if 1
//static atomic_t v;
static atomic_t v = ATOMIC_INIT(1);
#endif


struct pin_desc pins_desc[4] = {
    {S3C2410_GPF(0),  0x01},
    {S3C2410_GPF(2),  0x02},
    {S3C2410_GPG(3),  0x03},
    {S3C2410_GPG(11), 0x04},
};

static irq_handler_t buttons_irq(int irq, void *dev_id)
{
    struct pin_desc* pindesc = (struct pin_desc*)dev_id;

    unsigned int pinval = s3c2410_gpio_getpin(pindesc->pin);

    if (pinval)
    {
        key_val = 0x80 | (pindesc->key_val);
    }
    else
    {
        key_val = pindesc->key_val;
    }

    ev_press = 1;
    wake_up_interruptible(&buttons_wq);

    kill_fasync(&button_fasync, SIGIO, POLL_IN);
    
    return IRQ_HANDLED;
}

static int keys_open(struct inode *inode, struct file *filp)
{
#if 0
    if ((filp->f_flags) & O_NONBLOCK)   //非阻塞打开
    {
        if (down_trylock(&sem))         //不会休眠 信号里不可获取时 返回非零值
            return -EBUSY;
    }
    else
    {
        if (down_interruptible(&sem))  //会引起休眠 等待获取信号量
            return -ERESTARTSYS;
    }
#endif

#if 0
    if ((filp->f_flags) & O_NONBLOCK)
        {
            if (mutex_trylock(&lock))
                return -EBUSY;
        }
        else
        {
            if (mutex_lock_interruptible(&lock))
                return -ERESTARTSYS;
        }
#endif

#if 1
    if (! atomic_dec_and_test(&v))      //自减并测试 为0 返回true
    {
        atomic_inc(&v);                 //如果不是0 表示设备已被打开
        return -EBUSY;
    }
#endif

    request_irq(IRQ_EINT0,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S2", &pins_desc[0]);
    request_irq(IRQ_EINT2,  buttons_irq, IRQ_TYPE_EDGE_BOTH, "S3", &pins_desc[1]);
    request_irq(IRQ_EINT11, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S4", &pins_desc[2]);
    request_irq(IRQ_EINT19, buttons_irq, IRQ_TYPE_EDGE_BOTH, "S5", &pins_desc[3]);

    return 0;
}

static int keys_fasync(int fd, struct file *filp, int mode)
{
    return fasync_helper(fd, filp, mode, &button_fasync);
}

static int keys_close(struct inode *inode, struct file *filp)
{
    free_irq(IRQ_EINT0,  &pins_desc[0]);
    free_irq(IRQ_EINT2,  &pins_desc[1]);
    free_irq(IRQ_EINT11, &pins_desc[2]);
    free_irq(IRQ_EINT19, &pins_desc[3]);
    
#if 0
    up(&sem);      //释放信号量
#endif

#if 0
    mutex_unlock(&lock);      //释放互斥锁
#endif

#if 1
    atomic_inc(&v);
#endif 

    return keys_fasync(-1, filp, 0);
}

static ssize_t keys_read(struct file *filp, char __user *buf, size_t size, loff_t *loft)
{
    if ( size != 1)
        return -EINVAL;

    if ((filp->f_flags) & O_NONBLOCK)          //非阻塞操作
    {
        if (ev_press == 0)
            return -EAGAIN;
    }
    else
    {
        wait_event_interruptible(buttons_wq, ev_press != 0);
    }
    
    copy_to_user(buf, &key_val, 1);
    ev_press = 0;
    
    return 1;
}

static unsigned int keys_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;

    poll_wait(filp, &buttons_wq, wait);

    if(ev_press)
        mask |= POLLIN | POLLRDNORM;
    
    return mask;
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_open,
    .read = keys_read,
    .poll = keys_poll,
    .fasync = keys_fasync,
    .release = keys_close,
};

static int __init keys_init(void)
{
    major = register_chrdev(0, "keys_old_drv", &keys_fops);
    keys_old_cls = class_create(THIS_MODULE, "keys_old_cls");
    device_create(keys_old_cls, NULL, MKDEV(major, 0), NULL, "buttons");

    //sema_init(&sem, 1);
    //mutex_init(&lock);
    //atomic_set(&v, 1);

    return 0;
}

static void __exit keys_exit(void)
{
    device_destroy(keys_old_cls, MKDEV(major, 0));
    class_destroy(keys_old_cls);
    unregister_chrdev(major, "keys_old_drv");
}

module_init(keys_init);
module_exit(keys_exit);

MODULE_LICENSE("GPL");


