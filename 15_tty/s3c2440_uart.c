


static struct uart_driver s3c24xx_uart_drv = {
    .owner = THIS_MODULE,
    .driver_name = "s3c2410_serial",
    .nr = 4,
    .cons = &s3c24xx_serial_console,
    .dev_name = "ttySAC",
    .major = 204,
    .minor = 64,
};

/* module initialisation code */

static int __init s3c24xx_serial_modinit(void)
{
    int ret;

    ret = uart_register_driver(&s3c24xx_uart_drv);
    if (ret < 0) {
        printk(KERN_ERR "failed to register UART driver\n");
        return -1;
    }

    return platform_driver_register(&samsung_serial_driver);
}


