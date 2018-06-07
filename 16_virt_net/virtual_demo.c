#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/timer.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>



#define SNULL_TIMEOUT 10    //in jiffies

enum SUNLL_STATS {
    SNULL_TX_INTR = 0x0001,
    SNULL_RX_INTR = 0x0002
};


//定义两个设备
struct net_device *snull_devs[2];

static int timeout = SNULL_TIMEOUT;
static int lockup = 0;

module_param(timeout, int, 0644);
MODULE_PARM_DESC(timeout, " Use for tx timeout default: 10 jiffies");

module_param(lockup, int, 0644);
MODULE_PARM_DESC(lockup, " Use for simulating a dropped transmit interrupt default: 0 (no dropped)");


static unsigned char mac_addr1[ETH_ALEN] = {0x08, 0x89, 0x11, 0x45, 0x55, 0x66};
static unsigned char mac_addr2[ETH_ALEN] = {0x08, 0x89, 0x11, 0x45, 0x55, 0x88};


//网络设备结构体,作为net_device->priv  
struct snull_priv {  
    int status;                     //网络设备的状态信息，是发完数据包，还是接收到网络数据包  
    int rx_packetlen;               //接收到的数据包长度  
    u8 *rx_packetdata;              //接收到的数据  
    int tx_packetlen;               //发送的数据包长度  
    u8 *tx_packetdata;              //发送的数据  
    struct sk_buff *skb;            //socket buffer结构体，网络各层之间传送数据都是通过这个结构体来实现的  
    spinlock_t lock;                //自旋锁  
};


static int snull_open(struct net_device *dev)
{
    printk("call %s()\n", __func__);

    //分配一个硬件地址 ETH_ALEN(6)是网络设备硬件地址的长度
    //memcpy(dev->dev_addr, "/0SNUL0", ETH_ALEN);
    if (dev == snull_devs[0])
        memcpy(dev->dev_addr, mac_addr1, ETH_ALEN);
    if (dev == snull_devs[1])
        memcpy(dev->dev_addr, mac_addr2, ETH_ALEN);

    // 激活设备传输队列
    netif_start_queue(dev);

    return 0;
}

static int snull_reslease(struct net_device *dev)
{
    printk("call %s()\n", __func__);

    //停止设备传输队列
    netif_stop_queue(dev);

    return 0;
}

static void snull_rx(struct net_device *dev, int len, char *buf)
{
    struct sk_buff *skb;

    //分配一个skb 并初始化skb->data skb->tail skb->head
    skb = dev_alloc_skb(len + 2);
    if (!skb)
    {
        if (printk_ratelimit())
            printk("snull rx: low no mem - packet dropped\n");
        dev->stats.rx_dropped++;
        return;
    }

    //由于以太网头ETH_HLEN为14字节 为保持IP头在以太网头后16字节对齐
    //故在数据包之前保留2字节 即skb->data核skb->len均往下移动len长度
    skb_reserve(skb, 2);

    //返回skb_put()调用之前的skb->tail
    //修改数据区的skb->tail指针，往下移动len
    
    //数据拷贝过程可这样理解
    //<1>先扩展skb空间
    //<2>把数据拷贝至新扩展的空间
    //unsigned char *tail_tmp = skb_put(skb, len);
    //memcpy(tail_tmp, buf, len)
    memcpy(skb_put(skb, len), buf, len);

    skb->dev = dev;
    skb->protocol = eth_type_trans(skb, dev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;

    dev->stats.rx_packets++;
    dev->stats.rx_bytes += len;
    //保存数据接收时间
    dev->last_rx = jiffies;

    netif_rx(skb);
}

//中断处理 虚拟网络没有真正的硬件中断 只是模拟中断 在发送完数据包后 产生中断
//有新数据到达网络口时 也产生中断 
static void snull_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    int statusword; //用来标识是发送完毕还是接收到新的数据包
    struct snull_priv *priv;
    struct net_device *dev = (struct net_device *)dev_id;

    if (!dev)
        return;

    priv = netdev_priv(dev);
    spin_lock(&priv->lock);

    statusword = priv->status;
    //如果是接收
    if (statusword & SNULL_RX_INTR)
    {
        snull_rx(dev, priv->rx_packetlen, priv->rx_packetdata);
    }
    
    //如果是发送完毕
    if (statusword & SNULL_TX_INTR)
    {
        dev->stats.tx_packets++;
        dev->stats.tx_bytes += priv->tx_packetlen;
        dev_kfree_skb_any(priv->skb);   //释放skb套接字缓冲区
    }

    spin_unlock(&priv->lock);
}


//模拟从一个网络向另外一个网络发送数据包
static void snull_hw_tx(char *buf, int len, struct net_device *dev)
{
    struct iphdr *ih;           //ip头部
    struct net_device *dest;    //目标设备结构体
    u32 *saddr, *daddr;         //源设备与目的设备地址
    struct snull_priv *priv;

    if (len < sizeof(struct ethhdr) + sizeof(struct iphdr)) {
        printk("snull: Hmm... packet too short (%i octets)\n", len);
        return;
    }

    //在同一台机器上模拟两个网络 不同的网段地址
    ih = (struct iphdr *)(buf + sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;
    //源192.168.10.1(local0) --> 目的192.168.10.2(remote0)

    ((u8 *)saddr)[2] ^= 1;
    ((u8 *)daddr)[2] ^= 1;
    //修改后: 源192.168.11.1(remote1) --> 目的192.168.11.2(local1)
    
    ih->check = 0;      //rebuild the checksum
    ih->check = ip_fast_csum((unsigned char *)ih, ih->ihl);

    //ntohl()以主机字节序表示32位整数
    //ntohs()以主机字节序表示16位整数
    if (dev == snull_devs[0])
    {
        //过滤广播打印 大端模式 广播时[0][1][2][3]=00 00 10 00 其他时[0][1][2][3]=192 168 10 01
        if ( ((u8 *)saddr)[0] != ((u8 *)saddr)[1] )
        {
            //16bit 0~65535 5位十进制数 source/dest表示端口号
            printk("[sn0] ip:%08x, port:%05i --> ip:%08x, port:%05i\n", 
                ntohl(ih->saddr), ntohs(((struct tcphdr *)(ih+1))->source),
                ntohl(ih->daddr), ntohs(((struct tcphdr *)(ih+1))->dest));
       }

        //如果dev是0 那么dest就是1
        dest = snull_devs[1];
    }

    if (dev == snull_devs[1])
    {
        if ( ((u8 *)saddr)[0] != ((u8 *)saddr)[1] )
        {
            printk("ip:%08x, port:%05i --> ip:%08x, port:%05i [sn1]\n", 
                ntohl(ih->daddr), ntohs(((struct tcphdr *)(ih+1))->dest),
                ntohl(ih->saddr), ntohs(((struct tcphdr *)(ih+1))->source));
        }

        //如果dev是1 那么dest就是0
        dest = snull_devs[0];
    }

    //获取目标中的priv
    priv = netdev_priv(dest);
    priv->status = SNULL_RX_INTR;
    priv->rx_packetlen = len;
    priv->rx_packetdata = buf;
    //数据接收终端
    snull_interrupt(0, dest, NULL);

    //获取源中的priv
    priv = netdev_priv(dev);
    priv->status = SNULL_TX_INTR;
    priv->tx_packetlen = len;
    priv->tx_packetdata = buf;

    //simulate a dropped transmit interrupt
    //lockup=n 将在每传输n个数据包之后 模拟一次硬件锁住
    if (lockup && ((dest->stats.tx_packets + 1) % lockup) == 0)
    {
        netif_stop_queue(dev);
        printk("Simulate lockup at %ld, txp %ld\n", jiffies, dest->stats.tx_packets);
    }
    else
    {
        //发送完成中断 
        snull_interrupt(0, dev, NULL);
    }
}

static void snull_tx_timeout(struct net_device *dev);

static netdev_tx_t snull_tx(struct sk_buff *skb, struct net_device *dev)
{
    int len;
    char *data;
    struct snull_priv *priv = netdev_priv(dev);

    if (skb == NULL)
    {
        printk("tint for %p, skb %p\n", dev, skb);
        snull_tx_timeout(dev);
        if (skb == NULL)
            return 0;
    }

    //ETH_ZLEN(60)是所发的最小数据包长度
    len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
    data = skb->data;
    //保存当前发送时间
    dev->trans_start = jiffies;
    //netif_trans_update(dev);
    //在发送完成后释放skb
    priv->skb = skb;

    //真正的发送函数
    snull_hw_tx(data, len, dev);
    return NETDEV_TX_OK;
}

static void snull_tx_timeout(struct net_device *dev)
{
    struct snull_priv *priv = netdev_priv(dev);
    
    printk("call %s()\n", __func__);
    //printk("Transmit timeout at %ld, latency %ld\n", jiffies, jiffies - dev->trans_start);
    priv->status = SNULL_TX_INTR;
    snull_interrupt(0, dev, NULL);  //超时后 填补"丢失"的中断 释放该次skb
    
    dev->stats.tx_errors++;         //统计错误数
    netif_wake_queue(dev);          //重新启动发送队列
}

static const struct net_device_ops virt_netdev_ops = {
    .ndo_open       = snull_open,
    .ndo_stop       = snull_reslease,
    .ndo_start_xmit = snull_tx,             //发包函数
    .ndo_tx_timeout = snull_tx_timeout,     //timeout超时被调用
};

static void snull_init(struct net_device *dev)
{
    struct snull_priv *priv;

    printk("call %s()\n", __func__);

    // 初始化以太网的公用成员
    ether_setup(dev);
    
    dev->netdev_ops     = &virt_netdev_ops;
    // dev->ethtool_ops    = &virt_ethtool_ops;

    //超时值 每经过timeout时间调用dev_watchdog() 
    //判断上个包的传输是否发生问题 如果时调用snull_tx_timeout()
    dev->watchdog_timeo = timeout;

    //不使用以太网的ARP协议
    dev->flags |= IFF_NOARP;

    priv = netdev_priv(dev);
    spin_lock_init(&priv->lock);
}

static int __init snull_init_module(void)
{
    int i;
    int result;
    struct net_device *dev;

    dev = alloc_netdev(sizeof(struct snull_priv), "sn0", snull_init);
    //dev = alloc_netdev(sizeof(struct snull_priv), "sn0", NET_NAME_UNKNOWN, snull_init);
    if (!dev)
    {
        printk(KERN_ERR "Error: alloc net device 0 fialed!\n");
        return -ENOMEM;
    }
    snull_devs[0] = dev;
    
    dev = alloc_netdev(sizeof(struct snull_priv), "sn1", snull_init);
    //dev = alloc_netdev(sizeof(struct snull_priv), "sn1", NET_NAME_UNKNOWN, snull_init);
    if (!dev)
    {
        printk(KERN_ERR "Error: alloc net device 1 fialed!\n");
        free_netdev(snull_devs[0]);
        return -ENOMEM;
    }
    snull_devs[1] = dev;

    for (i = 0; i < 2; i++)
    {
        dev = snull_devs[i];
        result = register_netdev(dev);
        if (result)
        {
            printk(KERN_ERR "Error: %d registering device %s failed\n", result, dev->name);
            goto error;
        }
    }

    return 0;

error:
    for (i = i - 1; i >=0; i--)
    {
        unregister_netdev(snull_devs[i]);
    }
    free_netdev(snull_devs[1]);
    free_netdev(snull_devs[0]);

    return result;
}

static void __exit snull_cleanup(void)
{
    int i;

    for (i = 1; i >= 0; i--)
    {
        unregister_netdev(snull_devs[i]);
        free_netdev(snull_devs[i]);
    }
}


module_init(snull_init_module);
module_exit(snull_cleanup);

MODULE_DESCRIPTION("virtual net demo");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


/*
 * 使用C类IP 否则需要添加子网掩码255.255.255.0
 * C类ip 192.168.0.0 ~ 192.168.255.255
 *
 * <1>设置sn0 sn1所属网络(不通网络段) /etc/networks 同时即可用名字来指代网络
 * sn0 192.168.10.0
 * sn1 192.168.11.0
 * 注意 10 ^ 1 = 11, 11 ^ 1 = 10
 *
 * <2>设置ip /etc/hosts
 * 192.168.10.1 local0
 * 192.168.10.2 remote0
 * 
 * 192.168.11.2 local1
 * 192.168.11.1 remote1
 *
 * <3>启动网络 设置接口
 * ifconfig sn0 local0
 * ifconfig sn1 local1
 */

