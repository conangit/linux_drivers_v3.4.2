在文件arch/arm/mach-s3c24xx/mach-smdk2440.c文件中

模拟at24c02添加client
/*at24c02*/
static struct at24_platform_data at24c08 = {
    .byte_len   = SZ_8K / 8,
    .page_size  = 16,
};

static struct i2c_board_info mini2440_i2c_devs[] __initdata = {
    {
        I2C_BOARD_INFO("24c08", 0x50),
        .platform_data = &at24c08,
    },
};



/*at24c256*/
static struct at24_platform_data at24c256 = {
    .byte_len   = SZ_256K / 8,
    .page_size  = 64,
    .flags      = AT24_FLAG_ADDR16 | AT24_FLAG_IRUGO,
};

static struct i2c_board_info smdk2440_i2c_devs[] __initdata = {
    {
        I2C_BOARD_INFO("24c256", 0x50), //1010_0_A1_A0_R/W
        .platform_data = &at24c256,
    },

static void __init smdk2440_machine_init(void)
{
    ....
    i2c_register_board_info(0, smdk2440_i2c_devs, ARRAY_SIZE(smdk2440_i2c_devs));
    ....
}

则在目录/sys/bus/i2c/devices看到0-0050设备
