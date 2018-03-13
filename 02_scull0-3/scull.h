#ifndef __SCULL_H
#define __SCULL_H

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/semaphore.h>
#include <linux/device.h>

//主设备号
#define SCULL_MAJOR    0
//量子大小
#define SCULL_QUANTUM  4096
//量子集大小
#define SCULL_QSET     1000
//设备数量
#define SCULL_NR_DEVS  4

struct scull_qset_t {
    void **data;
    struct scull_qset_t *next;
};


// scull device
struct scull_dev {
    struct scull_qset_t *data;      //指向第一个量子集的指针
    int qset;                       //当前量子集大小
    int quantum;                    //当前量子的大小
    unsigned long size;             //当前设备数据的字节数
    unsigned int access_key;
    struct semaphore sem;           //互斥信号量
    struct cdev cdev;
};


#endif