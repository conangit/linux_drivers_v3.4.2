#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/crc32.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/dm9000.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/slab.h>

#include <asm/delay.h>
#include <asm/irq.h>
#include <asm/io.h>

#include "dm9000.h"


static int __devinit dm9000_probe(struct platform_device *pdev)
{
    printk("%s()\n", __func__);

    return 0;
}




static int __devexit dm9000_remove(struct platform_device *pdev)
{
    printk("%s()\n", __func__);


    return 0;
}

static struct platform_driver dm9000_driver = {
    .driver = {
        .name  = "dm9000_ext",
        .owner = THIS_MODULE,
    },
    .probe   = dm9000_probe,
    .remove  = __devexit_p(dm9000_remove),
};


//add by lihong
struct s3c_mem_ctrl  {
    unsigned long bwscon; 
    unsigned long bankcon0; 
    unsigned long bankcon1; 
    unsigned long bankcon2; 
    unsigned long bankcon3; 
    unsigned long bankcon4; 
    unsigned long bankcon5; 
    unsigned long bankcon6; 
    unsigned long bankcon7; 
    unsigned long refresh; 
    unsigned long banksize; 
    unsigned long mrsrb6; 
    unsigned long mrsrb7;
};

static struct s3c_mem_ctrl *s3c_mem_ctrl_p;

#if 0
    #define B4_BWSCON             (DW16 + WAIT + UBLB)
    
    #define B4_Tacs                 0x0     /* 0clk */
    #define B4_Tcos                 0x3     /* 4clk */
    #define B4_Tacc                 0x7     /* 14clk */
    #define B4_Tcoh                 0x1     /* 1clk */
    #define B4_Tah                  0x3     /* 4clk */
    #define B4_Tacp                 0x6     /* 6clk */
    #define B4_PMC                  0x0     /* normal */
#endif

static int dm9000_ext_init(void)
{
    u32 val1;
    u16 val2;

    s3c_mem_ctrl_p = (struct s3c_mem_ctrl *)ioremap(0x48000000, sizeof(struct s3c_mem_ctrl));
    if (!s3c_mem_ctrl_p)
        return -ENOMEM;

    //[13:12]=01 16bit
    //[14]=0 WAIT disable 
    //[15]=0  Not using UB/LB
    val1 = ioread32(&s3c_mem_ctrl_p->bwscon);
    val1 &= ~(0xf << 12);
    val1 |= (1 << 12);
    iowrite32(val1, &s3c_mem_ctrl_p->bwscon);

    //[14:13]=0; [12:11]=0; [10:8]=1; [7:6]=1; [5:4]=0; [3:2]=0; [1:0]=0;
    //[10:8] >= 3 从时序图来看可以取临界的0值，但是编译运行后出错
    val2 = ioread16(&s3c_mem_ctrl_p->bankcon3);
    printk("original bankcon3 = 0x%x\n", val2);
    val2 &= ~0xffff;
    val2 = (7 << 8) | (1 << 6);
    iowrite16(val2, &s3c_mem_ctrl_p->bankcon3);
    val2 = ioread16(&s3c_mem_ctrl_p->bankcon3);
    printk("modified bankcon3 = 0x%x\n", val2);

    return 0;
}

static void dm9000_me_ext_exit(void)
{
    iounmap(s3c_mem_ctrl_p);
}

static int __init dm9000_init(void)
{
    int ret;

    ret = dm9000_ext_init();
    if (ret)
    {
        return ret;
        printk(KERN_ERR "Error: no memory for device control\n");
    }

    return platform_driver_register(&dm9000_driver);
}

static void __exit dm9000_cleanup(void)
{
    platform_driver_unregister(&dm9000_driver);
    dm9000_me_ext_exit();
}

module_init(dm9000_init);
module_exit(dm9000_cleanup);

MODULE_DESCRIPTION("dm9000 driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");



