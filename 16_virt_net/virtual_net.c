#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/timer.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/ip.h>



struct net_device *vnet_dev;
int timeout = 10;
struct timer_list rx_timer;

static void rx_irq(unsigned long data)
{
    struct net_device *dev = (struct net_device *)data;
    struct sk_buff *skb;
    const char *rx_data = "ABCDEFG";

    skb = dev_alloc_skb(512);

    memcpy(skb->data, rx_data, 7);

    skb->protocol = eth_type_trans(skb, dev);

    dev->stats.rx_packets++;
    dev->stats.rx_bytes += 7;

    dev->last_rx = jiffies;

    netif_rx(skb);

    mod_timer(&rx_timer, jiffies + HZ * 2);
}

static int vnet_open(struct net_device *dev)
{
    init_timer(&rx_timer);
    rx_timer.data = (unsigned long)dev;
    rx_timer.function = rx_irq;
    rx_timer.expires  = jiffies + HZ * 5;
    add_timer(&rx_timer);

    // 激活设备发送队列
    netif_start_queue(dev);
    return 0;
}

static int vnet_stop(struct net_device *dev)
{
    //停止设备传输包
    netif_stop_queue(dev);
    del_timer(&rx_timer);
    return 0;
}

static void emulator_rx_packet(struct sk_buff *skb, struct net_device *dev)
{
    /* 参考LDD3 */
    unsigned char *type;
    struct iphdr *ih;
    __be32 *saddr, *daddr, tmp;
    unsigned char   tmp_dev_addr[ETH_ALEN];
    struct ethhdr *ethhdr;
    
    struct sk_buff *rx_skb;
        
    // 从硬件读出/保存数据
    /* 对调"源/目的"的mac地址 */
    ethhdr = (struct ethhdr *)skb->data;
    memcpy(tmp_dev_addr, ethhdr->h_dest, ETH_ALEN);
    memcpy(ethhdr->h_dest, ethhdr->h_source, ETH_ALEN);
    memcpy(ethhdr->h_source, tmp_dev_addr, ETH_ALEN);

    /* 对调"源/目的"的ip地址 */    
    ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
    saddr = &ih->saddr;
    daddr = &ih->daddr;

    tmp = *saddr;
    *saddr = *daddr;
    *daddr = tmp;
    
    //((u8 *)saddr)[2] ^= 1; /* change the third octet (class C) */
    //((u8 *)daddr)[2] ^= 1;
    type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);
    //printk("tx package type = %02x\n", *type);
    // 修改类型, 原来0x8表示ping
    *type = 0; /* 0表示reply */
    
    ih->check = 0;         /* and rebuild the checksum (ip needs it) */
    ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);
    
    // 构造一个sk_buff
    rx_skb = dev_alloc_skb(skb->len + 2);
    skb_reserve(rx_skb, 2); /* align IP on 16B boundary */  
    memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

    /* Write metadata, and then pass to the receive level */
    rx_skb->dev = dev;
    rx_skb->protocol = eth_type_trans(rx_skb, dev);
    rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
    dev->stats.rx_packets++;
    dev->stats.rx_bytes += skb->len;

    // 提交sk_buff
    netif_rx(rx_skb);
    dev_kfree_skb_any(skb);
}

static netdev_tx_t vnet_tx(struct sk_buff *skb, struct net_device *dev)
{
    static unsigned int cnt = 0;
    int ret = NETDEV_TX_OK;

    netif_stop_queue(dev);

    // 内部回环 首先 ifconfig lo down
    emulator_rx_packet(skb, dev);

    dev->trans_start = jiffies;

    netif_wake_queue(dev);

    dev->stats.rx_packets++;
    dev->stats.rx_bytes += skb->len;

    printk(KERN_DEBUG "[%u] %s()\n", ++cnt, __func__);

    return ret;
}


static const struct net_device_ops virt_netdev_ops = {
    .ndo_open       = vnet_open,
    .ndo_stop       = vnet_stop,
    .ndo_start_xmit = vnet_tx,
};

static const struct ethtool_ops virt_ethtool_ops = {

};

static void vnet_setup(struct net_device *dev)
{
    // 初始化以太网的公用成员
    ether_setup(dev);

    dev->netdev_ops     = &virt_netdev_ops;
    dev->ethtool_ops    = &virt_ethtool_ops;
    dev->watchdog_timeo = timeout;

    // MAC地址
    dev->dev_addr[0] = 0x08;
    dev->dev_addr[1] = 0x89;
    dev->dev_addr[2] = 0x08;
    dev->dev_addr[3] = 0x18;
    dev->dev_addr[4] = 0x28;
    dev->dev_addr[5] = 0x38;

    dev->flags |= IFF_NOARP;
}


static int __init vnet_init(void)
{
    int ret;
    
    vnet_dev = alloc_netdev(0, "vnet%d", vnet_setup);
    if (!vnet_dev)
    {
        printk(KERN_ERR "alloc netdev fialed!\n");
        return -ENOMEM;
    }

    ret = register_netdev(vnet_dev);
    if (ret)
    {
        printk(KERN_ERR "register netdev fialed!\n");
    }

    return ret;
}

static void __exit vnet_exit(void)
{
    unregister_netdev(vnet_dev);
    free_netdev(vnet_dev);
}

module_init(vnet_init);
module_exit(vnet_exit);

MODULE_DESCRIPTION("virtual net driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


