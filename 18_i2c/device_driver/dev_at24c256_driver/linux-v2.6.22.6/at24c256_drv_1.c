#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };
static unsigned short forces_addr[] = {ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short * forces[] = {forces_addr, NULL};

#if 0
/* Addresses to scan */
static unsigned short normal_addr[] = { 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, I2C_CLIENT_END };
#endif

static struct i2c_client_address_data addr_data = {
    // 要发出S信号和地址信号并得到ACK 才能确认是否存在这个设备
    .normal_i2c = normal_addr,
    .probe      = ignore,
    .ignore     = ignore,
    // 强制认为存在这个设备
    // .forces     = forces,
};

static struct i2c_driver at24c256_driver;

static int at24c256_probe(struct i2c_adapter *adap, int addr, int kind)
{
    struct i2c_client *new_client;
    
    printk("%s()\n", __func__);

    new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);
    new_client->addr = addr;
    new_client->adapter = adap;
    new_client->driver = &at24c256_driver;
    strcpy(new_client->name, "at24c256");
    i2c_attach_client(new_client);
    
    return 0;
}

static int at24c256_attach(struct i2c_adapter *adap)
{
    return i2c_probe(adap, &addr_data, at24c256_probe);
}

static int at24c256_detach(struct i2c_client *client)
{
    printk("%s()\n", __func__);
    i2c_detach_client(client);
    return 0;}


static struct i2c_driver at24c256_driver = {
    .driver = {
        .name   = "at24c256",
    },
    // <linux/i2c-id.h>
    // .id = 
    .attach_adapter = at24c256_attach,
    .detach_client  = at24c256_detach,
};


static int __init at24c256_init(void)
{
    return i2c_add_driver(&at24c256_driver);
}

static void __exit at24c256_exit(void)
{
    i2c_del_driver(&at24c256_driver);
}


module_init(at24c256_init);
module_exit(at24c256_exit);

MODULE_DESCRIPTION("eeprom at24c256 driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lihongwqp@163.com");
MODULE_VERSION("v1.0.0");


