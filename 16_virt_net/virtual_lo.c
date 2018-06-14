#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/timer.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>


static int timeout = 5;
//用于检测网络状态
static struct timer_list timer;


static void netlink_status(unsigned long data)
{
    struct net_device *dev = (struct net_device *)data;

    dev->flags |= IFF_UP;

    //link down
    //unsigned char link = 0;
    //dev->flags |= IFF_RUNNING;
    //此时将ping不通
    
    //link up
    unsigned char link = 1;
    dev->flags &= ~IFF_RUNNING;

    printk("%s() old_status: %s\n", __func__, netif_carrier_ok(dev) ? "link up" : "link down");

    //如果网络状态已是0 - link up
    if (!(dev->flags & IFF_UP))
        goto set_timer;

    //从硬件获取link状态
    //link = vlo_get_link(dev);

    if (link)
    {
        if (!(dev->flags & IFF_RUNNING))
        {
            netif_carrier_on(dev);
            dev->flags |= IFF_RUNNING;
            printk("%s(), %s: link up\n", __func__, dev->name);
        }
    }
    else
    {
        if (dev->flags & IFF_RUNNING)
        {
            netif_carrier_off(dev);
            dev->flags &= ~IFF_RUNNING;
            printk("%s(), %s: link down\n", __func__, dev->name);
        }
    }

set_timer:
    mod_timer(&timer, jiffies + HZ * 4);
}

static int vlo_open(struct net_device *dev)
{
#if 1
    //初始化timer用于检测链路状态
    init_timer(&timer);
    timer.data = (unsigned long)dev;
    timer.function = netlink_status;
    timer.expires = jiffies + HZ * 5;
    add_timer(&timer);
#endif

    // 激活设备发送队列
    netif_start_queue(dev);
    return 0;
}

static int vlo_stop(struct net_device *dev)
{
    //停止设备传输包
    netif_stop_queue(dev);
    del_timer(&timer);
    return 0;
}

static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *eh;                      //以太网头
    unsigned char tmp_dev_addr[ETH_ALEN];

    struct iphdr *ih;                       //ip头部
    u32 *tx_saddr, *tx_daddr;               //源设备与目的设备地址

    unsigned char *type;

    struct sk_buff *rx_skb;


    //对调"源/目的"的mac地址--同一网卡,可不做
#if 0
    eh = (struct ethhdr *)skb->data;
    memcpy(tmp_dev_addr, eh->h_dest, ETH_ALEN);
    memcpy(eh->h_dest, eh->h_source, ETH_ALEN);
    memcpy(eh->h_source, tmp_dev_addr, ETH_ALEN);
#endif

    //取得源(tx)ip地址
    ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
    tx_saddr = &ih->saddr;
    tx_daddr = &ih->daddr;
    printk("source: saddr:0x%08x, daddr:0x%08x\n", *tx_saddr, *tx_daddr);

    type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    //printk("tx package type = %02x\n", *type);
    //修改类型, 原来0x8表示ping
    //0表示reply
    *type = 0;

    //rebuild the checksum (ip needs it)
    ih->check = 0;
    ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
    
    //构造一个sk_buff
    rx_skb = dev_alloc_skb(skb->len + 2);
    //align IP on 16B boundary
    skb_reserve(rx_skb, 2);
    memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

    //Write metadata, and then pass to the receive level
    //对调"源/目的"的ip地址--设置rx ip地址
    ih = (struct iphdr *)(rx_skb->data + sizeof(struct ethhdr));
    ih->saddr = *tx_daddr;
    ih->daddr = *tx_saddr;
    printk("dest: saddr:0x%08x, daddr:0x%08x\n", ih->saddr, ih->daddr);
    
    rx_skb->dev = dev;
    rx_skb->protocol = eth_type_trans(rx_skb, dev);
    rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += rx_skb->len;
    dev->last_rx = jiffies;

    //提交sk_buff
    netif_rx(rx_skb);

    //发送完成 释放该skb
    dev->stats.tx_packets++;
    dev->stats.tx_bytes += skb->len;
    //dev->trans_start = jiffies;
    netif_trans_update(dev);
    dev_kfree_skb_any(skb);
}

static netdev_tx_t vlo_tx(struct sk_buff *skb, struct net_device *dev)
{
    int len;

    //len = skb->len < ETH_ZLEN ? ETH_ZLEN : skb->len;
    //skb->len = len;

    // 内部回环
    emulator_rx_packet(skb, dev);

    return NETDEV_TX_OK;
}

static void vlo_tx_timeout(struct net_device *dev)
{
    netif_wake_queue(dev);
}

static const struct net_device_ops vlo_ops = {
    .ndo_open       = vlo_open,
    .ndo_stop       = vlo_stop,
    .ndo_start_xmit = vlo_tx,
    .ndo_tx_timeout = vlo_tx_timeout,
};

static void vlo_setup(struct net_device *dev)
{
    // 初始化以太网的公用成员
    ether_setup(dev);

    dev->netdev_ops     = &vlo_ops;
    dev->watchdog_timeo = msecs_to_jiffies(timeout);

    // MAC地址
    dev->dev_addr[0] = 0x08;
    dev->dev_addr[1] = 0x89;
    dev->dev_addr[2] = 0x08;
    dev->dev_addr[3] = 0x18;
    dev->dev_addr[4] = 0x28;
    dev->dev_addr[5] = 0x38;

    dev->flags |= IFF_NOARP;
    //dev->flags |= IFF_LOOPBACK; //自动赋值IP 127.0.0.1
}


struct net_device *vlo_dev;

static int __init vlo_init(void)
{
    int ret;
    
    //vlo_dev = alloc_netdev(0, "vlo", vlo_setup);
    vlo_dev = alloc_netdev(0, "vlo", NET_NAME_UNKNOWN, vlo_setup);
    
    if (!vlo_dev)
    {
        printk(KERN_ERR "alloc netdev fialed!\n");
        return -ENOMEM;
    }

    ret = register_netdev(vlo_dev);
    if (ret)
    {
        printk(KERN_ERR "register netdev fialed!\n");
    }

    return ret;
}

static void __exit vlo_exit(void)
{
    unregister_netdev(vlo_dev);
    free_netdev(vlo_dev);
}

module_init(vlo_init);
module_exit(vlo_exit);

MODULE_DESCRIPTION("virtual net driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


