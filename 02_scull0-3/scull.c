
#include "scull.h"

//设备数量
int scull_nr_devs = SCULL_NR_DEVS;
//主设备号
int scull_major = SCULL_MAJOR;
//次设备号
int scull_minor = 0;
//量子大小
int scull_quantum = SCULL_QUANTUM;
//量子集大小
int scull_qset = SCULL_QSET;

//设备
struct scull_dev *devs;
//类
struct class *scull_cls;

module_param(scull_nr_devs, int, S_IRUGO|S_IWUSR);
module_param(scull_major, int, S_IRUGO|S_IWUSR);
module_param(scull_minor, int, S_IRUGO|S_IWUSR);
module_param(scull_quantum, int, S_IRUGO|S_IWUSR);
module_param(scull_qset, int, S_IRUGO|S_IWUSR);

/*
 * 释放整个数据区 简单遍历列表并且释放发现的任何量子和量子集
 *
 */
int scull_trim(struct scull_dev *dev)
{
    int i;
    int qest;
    struct scull_qset_t *dptr, *next;

    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);

    //设备量子集大小
    qest = dev->qset;

    //dptr指向第一个量子集
    for (dptr = dev->data; dptr; dptr = next)
    {
        if (dptr->data)     //量子集里有数据
        {
            //遍历当前量子集
            for (i = 0; i < qest; i++)
            {
                kfree(dptr->data[i]);       //释放当前量子集中的每个量子指针
            }
            //释放量子数组指针
            kfree(dptr->data);
            dptr->data = NULL;
        }
        //获取下一个量子集
        next = dptr->next;
        //释放当前量子集
        kfree(dptr);
    }

    dev->size = 0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;
    
    return 0;
}

/*
 * 返回设备dev的第n个量子集的指针 量子集不够就申请新的量子集
 */
struct scull_qset_t *scull_follow(struct scull_dev *dev, int n)
{
    //第一个量子集指针
    struct scull_qset_t *dptr = dev->data;

    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);

    //如果当前设备没有量子集 就申请第一个量子集
    if (!dptr)
    {
        //此时应该让dev的data指针指向第一个量子集
        dptr = dev->data = kmalloc(sizeof(struct scull_qset_t), GFP_KERNEL);
        if (!dptr)
            return NULL;
        memset(dptr, 0, sizeof(struct scull_qset_t));
    }

    while (n--)
    {
        //若量子集不够 申请新的量子集
        if (!dptr->next)
        {
            dptr->next = kmalloc(sizeof(struct scull_qset_t), GFP_KERNEL);
            if (!dptr->next)
                return NULL;
            memset(dptr->next, 0, sizeof(struct scull_qset_t));
        }
        //dptr指向下一量子集
        dptr = dptr->next;
        continue;
    }
    
    return dptr;
}

int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev;

    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);

    //通过struct scull_dev的成员struct cdev获得struct scull_dev本身
    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev;
    
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
    {
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
        
        scull_trim(dev);

        up(&dev->sem);
    }

    return 0;
}

int scull_release(struct inode *indoe, struct file *filp)
{
    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);
    return 0;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
    struct scull_dev *dev = filp->private_data;
    struct scull_qset_t *dptr;

    //量子大小
    int quantum = dev->quantum;
    //量子集大小
    int qset = dev->qset;
    //一个scull_qset_t节点数据域的总数据量(量子集的数据量)
    int itemsize = quantum * qset;

    int item, rset, s_pos, q_pos;
    ssize_t retval = 0;

    printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    //printk(KERN_WARNING "*f_ops:%lu\n", *f_ops);
    //printk(KERN_WARNING "dev->size:%ld\n", dev->size);
    //printk(KERN_WARNING "count0:%ld\n", count);

    //要读的位置超过的数据总量
    if (*f_ops >= dev->size)
        goto out;

    //printk(KERN_WARNING "count1:%ld\n", count);
    //要读的count超出size 截断count
    if (*f_ops + count > dev->size)
        count = dev->size - *f_ops;
    //printk(KERN_WARNING "count2:%ld\n", count);

    //第几个量子集
    item = (long)*f_ops / itemsize;
    //在量子集中的偏移量
    rset = (long)*f_ops % itemsize;
    
    //该量子集中的第几个量子
    s_pos = rset / quantum;
    //该量子集的第几个量子的量子内部的偏移
    q_pos = rset % quantum;

    //printk(KERN_DEBUG "item = %d\n", item);
    //printk(KERN_DEBUG "rset = %d\n", rset);
    //printk(KERN_DEBUG "s_pos = %d\n", s_pos);
    //printk(KERN_DEBUG "q_pos = %d\n", q_pos);

    //确定要读取的量子集指针
    dptr = scull_follow(dev, item);

    //printk(KERN_WARNING "line:%d\n", __LINE__);

    if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
        goto out;

//    if (dptr == NULL)
//        goto out;
//    printk(KERN_WARNING "line:%d\n", __LINE__);
//
//    if (!dptr->data)
//        goto out;
//    printk(KERN_WARNING "line:%d\n", __LINE__);
//
//    if (!dptr->data[s_pos])
//        goto out;
//    printk(KERN_WARNING "line:%d\n", __LINE__);

    //只在一个量子中读 如果count超出一个量子大小 截断count
    if (count > quantum - q_pos)
           count = quantum - q_pos;
    //printk(KERN_WARNING "count3:%d\n", count);

    //只在当前量子中进行数据的读取
    //成功返回零 否则返回拷贝成功的字节数
    if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count))
    {
        retval = -EFAULT;
        goto out;
    }

    //printk(KERN_WARNING "read data: %s\n", (char *)(dptr->data[s_pos] + q_pos));

    *f_ops += count;
    retval = count;

out:
    up(&dev->sem);
    return retval;

}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
    struct scull_dev *dev = filp->private_data;
    struct scull_qset_t *dptr;

    int qset = dev->qset;
    int quantum = dev->quantum;
    int itemsize = quantum * qset;

    int item, rset, s_pos, q_pos;
    ssize_t retval = -ENOMEM;

    printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);
    //printk(KERN_DEBUG "qset = %d\n", qset);
    //printk(KERN_DEBUG "quantum = %d\n", quantum);

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    item = (long)*f_ops / itemsize;
    rset = (long)*f_ops % itemsize;
    s_pos = rset / quantum;
    q_pos = rset % quantum;

    //printk(KERN_DEBUG "item = %d\n", item);
    //printk(KERN_DEBUG "rset = %d\n", rset);
    //printk(KERN_DEBUG "s_pos = %d\n", s_pos);
    //printk(KERN_DEBUG "q_pos = %d\n", q_pos);

    dptr = scull_follow(dev, item);
    if (dptr == NULL)
        goto out;

    //printk(KERN_DEBUG "sizeof(char *) = %d\n", sizeof(char *));

    if (!dptr->data)
    {
        //此处申请的为数组指针变量 在32位机上 sizeof(pointer) = 4
        dptr->data = kmalloc(qset * 4, GFP_KERNEL);
        if (!dptr->data)
            goto out;
        memset(dptr->data, 0, qset * 4);
    }

    if (!dptr->data[s_pos])
    {
        dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dptr->data[s_pos])
            goto out;
    }

    if (count > quantum - q_pos)
       count = quantum - q_pos;

    if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count))
    {
        retval = -EFAULT;
        goto out;
    }

    //printk(KERN_WARNING "write data: %s\n", (char *)(dptr->data[s_pos] + q_pos));

    *f_ops += count;
    retval = count;

    //更新文件大小
    if (dev->size < *f_ops)
        dev->size = *f_ops;

    //printk(KERN_WARNING "dev->size: %d\n", dev->size);

out:
    up(&dev->sem);
    return retval;

}

static const struct file_operations scull_fops = {
    .owner = THIS_MODULE,
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
    int err;
    dev_t devno;

    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);

    devno = MKDEV(scull_major, scull_minor + index);
    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_NOTICE "Error: %d adding scull-%d\n", err, index);
    }

    device_create(scull_cls, NULL, devno, NULL, "scull-%d", index);
}

void __exit scull_exit(void)
{
    int i = 0;
    dev_t devno;

    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);

    if (devs)
    {
        for (i = 0; i < scull_nr_devs; i++)
        {
            devno = MKDEV(scull_major, scull_minor + i);
            scull_trim(&devs[i]);
            cdev_del(&devs[i].cdev);
            device_destroy(scull_cls, devno);
        }
        kfree(devs);
    }

    class_destroy(scull_cls);
    
    devno = MKDEV(scull_major, scull_minor);
    unregister_chrdev_region(devno, scull_nr_devs);
}

int __init scull_init(void)
{
    int result = 0;
    int i = 0;
    dev_t devno;

    //printk(KERN_DEBUG "call func %s()\n", __FUNCTION__);
    
    if (scull_major)
    {
        devno = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(devno, scull_nr_devs, "scull");
    }
    else
    {
        result = alloc_chrdev_region(&devno, scull_minor, scull_nr_devs, "scull");
        scull_major = MAJOR(devno);
    }

    if (result < 0)
    {
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    }

    devs = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    if(!devs)
    {
        result = -ENOMEM;
        goto fail;
    }
    memset(devs, 0, scull_nr_devs * sizeof(struct scull_dev));

    scull_cls = class_create(THIS_MODULE, "scull_cls");

    for (i = 0; i < scull_nr_devs; i++)
    {
        devs[i].qset = scull_qset;
        devs[i].quantum = scull_quantum;
        //初始化互斥锁
        sema_init(&devs[i].sem, 1);
        scull_setup_cdev(&devs[i], i);
    }

    return result;
    
fail:
    scull_exit();
    return result;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_DESCRIPTION("scull driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihong");
MODULE_VERSION("v1.0.0");

    
