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
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/timer.h>



/*
 * keys按下低电平 弹起高电平
 */

#define KEYS_MAJOR 0
#define KEYS_MINOR 0
#define KEYS_NR_DEVS 4

/*
DEFINE_SEMAPHORE(sem);
DEFINE_MUTEX(lock);
atomic_t v = ATOMIC_INIT(1);
*/

struct key_dev {
    int status;                     //0 - 按下 1 - 弹起
    unsigned int pin;               //key对应的GPIO
    
    unsigned int irq;               //中断向量
    unsigned long irq_flags;        //中断方式
    char irq_name[16];              //中断名称

    int ev_flag;                            //dev等待队列condition
    wait_queue_head_t wq;                   //dev等待队列

    struct fasync_struct *async_queue;      //异步通知

    // struct semaphore sem;                   //信号量
    // struct mutex lock;                      //互斥体 (用于非阻塞时候 出错?)
    // atomic_t v;                             //原子变量
    spinlock_t t_lock;                      //自旋锁
    int open_cnt;                           //设备打开计数

    struct timer_list timer;                //防抖定时器

    struct cdev cdev;                       //dev字符设备
};

static struct class *keys_cls = NULL;
static struct key_dev *devs = NULL;

static unsigned int keys_major = KEYS_MAJOR;
static unsigned int keys_minor = KEYS_MINOR;
static unsigned int keys_nr_devs = KEYS_NR_DEVS;

module_param(keys_major, uint, S_IRUGO|S_IWUSR);
module_param(keys_minor, uint, S_IRUGO|S_IWUSR);

static irqreturn_t key_handle_irq(int irq, void *dev_id)
{
    struct key_dev *dev = (struct key_dev *)dev_id;

    mod_timer(&dev->timer, jiffies + HZ/50);   //20ms
    
    return IRQ_HANDLED;
}

/*
 * 此函数处于(软)中断上下文 遵行"三不"原则
 */
static void do_timer_func(unsigned long data)
{
    struct key_dev *dev = (struct key_dev *)data;

    printk(KERN_DEBUG "enter function %s()\n", __FUNCTION__);

    if (in_interrupt())
        printk(KERN_DEBUG "%s() is in interrupt process.\n", __FUNCTION__);

    dev->ev_flag = 1;
    wake_up_interruptible(&dev->wq);

    kill_fasync(&dev->async_queue, SIGIO, POLLIN);     //发送信号
}

static int keys_open(struct inode *inode, struct file *filp)
{
    struct key_dev *dev;
    int ret;
    unsigned int dev_minor;

    dev = container_of(inode->i_cdev, struct key_dev, cdev);
    filp->private_data = dev;

//信号量
#if 0
    if (filp->f_flags & O_NONBLOCK)         //非阻塞操作
    {
        // 获取不成功 立即返回 不会引起进程睡眠
        // 获取成功返回0 失败返回1
        if (down_trylock(&dev->sem))
            return -EBUSY;
    }
    else                                    //阻塞操作
    {
        // 获取不成功 进程睡眠 但可以被中断打断 成功返回0
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
    }
#endif

//互斥体
#if 0
    if (filp->f_flags & O_NONBLOCK)
    {
        // 获取成功返回1 失败返回0
        if (! mutex_trylock(&dev->lock))
            return -EBUSY;
    }
    else
    {
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
    }
#endif

//原子变量
#if 0
    //对v进行自减操作后v为0 返回true
    if (! atomic_dec_and_test(&dev->v))
    {
        atomic_inc(&dev->v);
        return -EBUSY;
    }
#endif

//自旋锁
#if 1
    spin_lock(&dev->t_lock);
    if (dev->open_cnt)
    {
        spin_unlock(&dev->t_lock);
        return -EBUSY;
    }
    dev->open_cnt++;
    spin_unlock(&dev->t_lock);
#endif

    dev_minor = iminor(inode);

    if (dev_minor == keys_minor)
    {
        dev->pin = S3C2410_GPF(0);
        dev->irq = IRQ_EINT0;                       //16
        strcpy(dev->irq_name, "key S2");
        printk(KERN_DEBUG "minor: 0\n");
    }
    else if (dev_minor == keys_minor + 1)
    {
        dev->pin = S3C2410_GPF(2);
        dev->irq = IRQ_EINT2;                       //18
        strcpy(dev->irq_name, "key S3");
        printk(KERN_DEBUG "minor: 1\n");
    }
    else if (dev_minor == keys_minor + 2)
    {
        dev->pin = S3C2410_GPG(3);
        dev->irq = IRQ_EINT11;                       //55
        strcpy(dev->irq_name, "key S4");
        printk(KERN_DEBUG "minor: 2\n");
    }
    else if (dev_minor == keys_minor + 3)
    {
        dev->pin = S3C2410_GPG(11);
        dev->irq = IRQ_EINT19;                       //63
        strcpy(dev->irq_name, "key S5");
        printk(KERN_DEBUG "minor: 3\n");
    }

    dev->status = 1;                         //默认弹起
    dev->irq_flags = IRQ_TYPE_EDGE_BOTH;

    // 初始化定时器资源
    init_timer(&dev->timer);
    dev->timer.function = do_timer_func;
    dev->timer.data = (unsigned long)dev;
    /*
     * 特别注意 一旦调用add_timer() 则定时器开始工作 
     * 而此时中断并未发生 却调用了do_timer_func() 导致访问资源出错
     */
    // add_timer(&dev->timer);

    ret = request_irq(dev->irq, key_handle_irq, dev->irq_flags, dev->irq_name, dev);
    if (ret < 0)
    {
        printk(KERN_ERR "Error: request irq error(%d)!\n", ret);
        goto fail;
    }

    printk(KERN_DEBUG "request irq success(%d).\n", ret);
    
    dev->ev_flag = 0;                  //默认阻塞打开

    return ret;
    
fail:
    // up(&dev->sem);
    // mutex_unlock(&dev->lock);
    // atomic_inc(&dev->v);
    return ret;
}

static int keys_fasync(int fd, struct file *filp, int mode)
{
    struct key_dev *dev = filp->private_data;

    return fasync_helper(fd, filp, mode, &dev->async_queue);
}

static int keys_release(struct inode *inode, struct file *filp)
{
    struct key_dev *dev = filp->private_data;

    free_irq(dev->irq, dev);

    del_timer(&dev->timer);
    
#if 0
    up(&dev->sem);
#endif

#if 0
    mutex_unlock(&dev->lock);
#endif

#if 0
    atomic_inc(&dev->v);
#endif

#if 1
    spin_lock(&dev->t_lock);
    dev->open_cnt--;
    spin_unlock(&dev->t_lock);
#endif

    return keys_fasync(-1, filp, 0);
}

static unsigned int keys_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    struct key_dev *dev = filp->private_data;

    // printk(KERN_DEBUG "enter function %s\n", __FUNCTION__);
    
    if (in_interrupt())
        printk(KERN_DEBUG "%s() is in interrupt process.\n", __FUNCTION__);
    else
        printk(KERN_DEBUG "%s() isn't in interrupt process.\n", __FUNCTION__);

    // 将进程挂入dev->wq等待队列
    poll_wait(filp, &dev->wq, wait);

    // 数据可读
    if (dev->ev_flag != 0)
        mask |= POLLIN | POLLRDNORM;

    return mask;
}

static ssize_t keys_read(struct file *filp, char __user *buf, size_t size, loff_t *f_ops)
{
    struct key_dev *dev = filp->private_data;
    unsigned int pinval;

    if (filp->f_flags & O_NONBLOCK)
    {
        if (dev->ev_flag == 0)
            return -EAGAIN;
    }
    else
    {
        //允许等待该信号的用户空间进程可被中断 如Ctrl + C
        if (wait_event_interruptible(dev->wq, dev->ev_flag != 0))
            return -ERESTARTSYS;
    }

    pinval = s3c2410_gpio_getpin(dev->pin);

    if (pinval)     //弹起
    {
        dev->status = 1;
    }
    else            //按下 
    {
        dev->status = 0;
    }

    if (copy_to_user(buf, &dev->status, sizeof(dev->status)))
        printk(KERN_ERR "Error:copy_to_user error!\n");

    dev->ev_flag = 0;

    return sizeof(dev->status);
}

static const struct file_operations keys_fops = {
    .owner = THIS_MODULE,
    .open = keys_open,
    .release = keys_release,
    .read = keys_read,
    .poll = keys_poll,
    .fasync = keys_fasync,
};

int key_setup(struct key_dev *dev, int index)
{   
    int ret;
    dev_t devno;

    devno = MKDEV(keys_major, keys_minor + index);
    cdev_init(&dev->cdev, &keys_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &keys_fops;
    ret = cdev_add(&dev->cdev, devno, 1);
    if (ret)
    {
        printk(KERN_ERR "Error:adding keys %d fail!\n", index);
        return ret;
    }

    device_create(keys_cls, NULL, devno, NULL, "button%d", index);

    init_waitqueue_head(&dev->wq);

    return ret;
}

int __init keys_drv_init(void)
{
    dev_t devno;
    int ret;
    int i;

    if (keys_major)
    {
        devno = MKDEV(keys_major, keys_minor);
        ret = register_chrdev_region(devno, keys_nr_devs, "keys");
    }
    else
    {
        ret = alloc_chrdev_region(&devno, keys_minor, keys_nr_devs, "keys");
        keys_major = MAJOR(devno);
    }

    if (ret < 0)
    {
        printk(KERN_ERR "Error:Can't get keys major %d!\n", keys_major);
        return ret;
    }

    devs = kmalloc(sizeof(struct key_dev) * keys_nr_devs, GFP_KERNEL);
    if (!devs)
    {
        ret = -ENOMEM;
        goto fail;
    }
    memset(devs, 0 , sizeof(struct key_dev) * keys_nr_devs);

    keys_cls = class_create(THIS_MODULE, "keys_cls");

    for (i = 0; i < keys_nr_devs; i++)
    {
        // sema_init(&devs[i].sem, 1);
        // mutex_init(&devs[i].lock);
        // atomic_set(&devs[i].v, 1);
        spin_lock_init(&devs[i].t_lock);
        devs[i].open_cnt = 0;

        ret = key_setup(&devs[i], i);
    }

    return ret;

fail:
    unregister_chrdev_region(devno, keys_nr_devs);
    return ret;
}

void __exit keys_drv_exit(void)
{
    int i;

    if (devs)
    {
        for (i = 0; i < keys_nr_devs; i++)
        {
            device_destroy(keys_cls, MKDEV(keys_major, keys_minor + i));
            cdev_del(&devs[i].cdev);
        }
        kfree(devs);
        devs = NULL;
    }

    class_destroy(keys_cls);
    unregister_chrdev_region(MKDEV(keys_major, keys_minor), keys_nr_devs);
}

module_init(keys_drv_init);
module_exit(keys_drv_exit);

MODULE_DESCRIPTION("keys driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.1.0");



