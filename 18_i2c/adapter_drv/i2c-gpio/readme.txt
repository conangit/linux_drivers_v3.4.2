使用内核自带的GPIO模拟I2C

源码：
drivers/i2c/busses/i2c-gpio.c
drivers/i2c/busses/i2c-gpio.h

内核配置:
Device Drivers  --->  
    <*> I2C support  --->
            I2C Hardware Bus support  ---> 
                 <*> GPIO-based bitbanging I2C
                 

添加单板信息:

struct i2c_gpio_platform_data jz2440_i2c_gpio_platform_data_1 = {
    .sda_pin = S3C2410_GPG(4),
    .scl_pin = S3C2410_GPF(1),
    // .udelay  = 20,           // 保持驱动默认5(100Khz)
    // .timeout = 100,          // 保持驱动默认100ms
    .sda_is_open_drain  = 1,
    .scl_is_open_drain  = 1,
    /*
     * 跟clock stretching相关
     * 当SCL线只具备output方向时 从设备无法拉低SCL线来暂停一个传输
     * 但是大都数从设备不具有驱动SCL功能 所以他们也无法stretching时钟
     */
    .scl_is_output_only = 0,    //SCL也为双向IO
};

static struct platform_device jz2440_i2c_gpio_bus_1 = {
    .name = "i2c-gpio",
    .id = 1,
    .dev = {
        .platform_data = &jz2440_i2c_gpio_platform_data_1,
    },
};

struct i2c_gpio_platform_data jz2440_i2c_gpio_platform_data_2 = {
    .sda_pin = S3C2410_GPG(2),
    .scl_pin = S3C2410_GPG(6),
    .sda_is_open_drain  = 0,    //外部有1K上拉
    .scl_is_open_drain  = 0,    //外部有1K上拉
    .scl_is_output_only = 0,
};

static struct platform_device jz2440_i2c_gpio_bus_2 = {
    .name = "i2c-gpio",
    .id = 2,
    .dev = {
        .platform_data = &jz2440_i2c_gpio_platform_data_2,
    },
};

