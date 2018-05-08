/* linux/arch/arm/mach-s3c2440/mach-smdk2440.c
 *
 * Copyright (c) 2004-2005 Simtec Electronics
 *  Ben Dooks <ben@simtec.co.uk>
 *
 * http://www.fluff.org/ben/smdk2440/
 *
 * Thanks to Dimity Andric and TomTom for the loan of an SMDK2440.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <mach/regs-gpio.h>
#include <mach/leds-gpio.h>

#include <plat/regs-serial.h>
#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>

#include <mach/idle.h>
#include <mach/fb.h>
#include <plat/iic.h>

#include <plat/s3c2410.h>
#include <plat/s3c244x.h>
#include <plat/clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>

#include <plat/common-smdk.h>
#include <plat/gpio-cfg.h>
#include <plat/devs.h>
#include <plat/pm.h>

#include "common.h"

/* DM9000 */
#if defined(CONFIG_DM9000) || defined(CONFIG_DM9000_MODULE)
#include <linux/dm9000.h>

#define MACH_SMDK2440_DM9K_BASE (S3C2410_CS4 + 0x300)

/* DM9000AEP 10/100 ethernet controller */
static struct resource smdk2440_dm9k_resource[] = {
    [0] = {
        .start = MACH_SMDK2440_DM9K_BASE,
        .end   = MACH_SMDK2440_DM9K_BASE + 3,
        .flags = IORESOURCE_MEM
    },
    [1] = {
        .start = MACH_SMDK2440_DM9K_BASE + 4,
        .end   = MACH_SMDK2440_DM9K_BASE + 7,
        .flags = IORESOURCE_MEM
    },
    [2] = {
        .start = IRQ_EINT7,
        .end   = IRQ_EINT7,
        .flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
    }
};

/* 
 * The DM9000 has no eeprom, and it's MAC address is set by
 * the bootloader before starting the kernel.
 */
static struct dm9000_plat_data smdk2440_dm9k_pdata = {
    .flags      = (DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM),
}; 

static struct platform_device smdk2440_device_eth = {
    .name       = "dm9000",
    .id     = -1,
    .resource   = smdk2440_dm9k_resource,
    .num_resources  = ARRAY_SIZE(smdk2440_dm9k_resource),
    .dev        = {
        .platform_data  = &smdk2440_dm9k_pdata,
    },
};
#endif /* CONFIG_DM9000 */


/* LED devices */
static struct s3c24xx_led_platdata smdk_pdata_led4 = {
    .gpio       = S3C2410_GPF(4),
    .flags      = S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
    .name       = "led4",
    .def_trigger    = "heartbeat",
};

static struct s3c24xx_led_platdata smdk_pdata_led5 = {
    .gpio       = S3C2410_GPF(5),
    .flags      = S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
    .name       = "led5",
    .def_trigger    = "nand-disk",
};

static struct s3c24xx_led_platdata smdk_pdata_led6 = {
    .gpio       = S3C2410_GPF(6),
    .flags      = S3C24XX_LEDF_ACTLOW | S3C24XX_LEDF_TRISTATE,
    .name       = "led6",
    .def_trigger    = "timer",
};

static struct platform_device smdk_led4 = {
    .name = "s3c24xx_led",
    .id = -1,
    .dev = {
        .platform_data = &smdk_pdata_led4,
    },
};

static struct platform_device smdk_led5 = {
    .name = "s3c24xx_led",
    .id = -1,
    .dev = {
        .platform_data = &smdk_pdata_led5,
    },
};

static struct platform_device smdk_led6 = {
    .name = "s3c24xx_led",
    .id = -1,
    .dev = {
        .platform_data = &smdk_pdata_led6,
    },
};
/* LED devices */


static struct map_desc __initdata smdk2440_iodesc[] = {
    /* ISA IO Space map (memory space selected by A24) */
    {
        .virtual    = (u32)S3C24XX_VA_ISA_WORD,
        .pfn        = __phys_to_pfn(S3C2410_CS2),
        .length     = 0x10000,
        .type       = MT_DEVICE,
    }, {
        .virtual    = (u32)S3C24XX_VA_ISA_WORD + 0x10000,
        .pfn        = __phys_to_pfn(S3C2410_CS2 + (1<<24)),
        .length     = SZ_4M,
        .type       = MT_DEVICE,
    }, {
        .virtual    = (u32)S3C24XX_VA_ISA_BYTE,
        .pfn        = __phys_to_pfn(S3C2410_CS2),
        .length     = 0x10000,
        .type       = MT_DEVICE,
    }, {
        .virtual    = (u32)S3C24XX_VA_ISA_BYTE + 0x10000,
        .pfn        = __phys_to_pfn(S3C2410_CS2 + (1<<24)),
        .length     = SZ_4M,
        .type       = MT_DEVICE,
    }
};

#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

static struct s3c2410_uartcfg __initdata smdk2440_uartcfgs[] = {
    [0] = {
        .hwport      = 0,
        .flags       = 0,
        .ucon        = 0x3c5,
        .ulcon       = 0x03,
        .ufcon       = 0x51,
    },
    [1] = {
        .hwport      = 1,
        .flags       = 0,
        .ucon        = 0x3c5,
        .ulcon       = 0x03,
        .ufcon       = 0x51,
    },
    /* IR port */
    [2] = {
        .hwport      = 2,
        .flags       = 0,
        .ucon        = 0x3c5,
        .ulcon       = 0x43,
        .ufcon       = 0x51,
    }
};

/* LCD driver info */
static struct s3c2410fb_display __initdata smdk2440_lcd_cfg  = {

    .lcdcon5 = S3C2410_LCDCON5_FRM565 |
               S3C2410_LCDCON5_INVVLINE |
               S3C2410_LCDCON5_INVVFRAME |
               S3C2410_LCDCON5_PWREN |
               S3C2410_LCDCON5_HWSWP,

    .type       = S3C2410_LCDCON1_TFT,

    .width      = 240,
    .height     = 320,

    .pixclock   = 166667, /* HCLK 60 MHz, divisor 10 */
    .xres       = 240,
    .yres       = 320,
    .bpp        = 16,
    .left_margin    = 20,
    .right_margin   = 8,
    .hsync_len  = 4,
    .upper_margin   = 8,
    .lower_margin   = 7,
    .vsync_len  = 4,
};

static struct s3c2410fb_mach_info __initdata smdk2440_fb_info = {
    .displays   = &smdk2440_lcd_cfg,
    .num_displays   = 1,
    .default_display = 0,

#if 0
    /* currently setup by downloader */
    .gpccon     = 0xaa940659,
    .gpccon_mask    = 0xffffffff,
    .gpcup      = 0x0000ffff,
    .gpcup_mask = 0xffffffff,
    .gpdcon     = 0xaa84aaa0,
    .gpdcon_mask    = 0xffffffff,
    .gpdup      = 0x0000faff,
    .gpdup_mask = 0xffffffff,
#endif

    .lpcsel     = ((0xCE6) & ~7) | 1<<4,
};

static struct platform_device __initdata *smdk2440_devices[] = {
    /* define by <plat/devs.h> */
    &s3c_device_wdt,
    &s3c_device_ohci,
    &s3c_device_iis,
    &s3c_device_i2c0,
    &s3c_device_lcd,
    &s3c_device_rtc,

#if defined(CONFIG_DM9000) || defined(CONFIG_DM9000_MODULE)
    &smdk2440_device_eth,
#endif

    &smdk_led4,
    &smdk_led5,
    &smdk_led6,
};

static void __init smdk2440_map_io(void)
{
    s3c24xx_init_io(smdk2440_iodesc, ARRAY_SIZE(smdk2440_iodesc));
    s3c24xx_init_clocks(12000000);
    s3c24xx_init_uarts(smdk2440_uartcfgs, ARRAY_SIZE(smdk2440_uartcfgs));
}

static void __init smdk2440_machine_init(void)
{
    /* Configure the LEDs (even if we have no LED support)*/
    s3c_gpio_cfgpin(S3C2410_GPF(4), S3C2410_GPIO_OUTPUT);
    s3c_gpio_cfgpin(S3C2410_GPF(5), S3C2410_GPIO_OUTPUT);
    s3c_gpio_cfgpin(S3C2410_GPF(6), S3C2410_GPIO_OUTPUT);
    s3c_gpio_cfgpin(S3C2410_GPF(7), S3C2410_GPIO_OUTPUT);

    s3c2410_gpio_setpin(S3C2410_GPF(4), 1);
    s3c2410_gpio_setpin(S3C2410_GPF(5), 1);
    s3c2410_gpio_setpin(S3C2410_GPF(6), 1);
    s3c2410_gpio_setpin(S3C2410_GPF(7), 1);
    
    s3c24xx_fb_set_platdata(&smdk2440_fb_info);
    s3c_i2c0_set_platdata(NULL);

    platform_add_devices(smdk2440_devices, ARRAY_SIZE(smdk2440_devices));

    /* common-smdk.c */
    smdk_machine_init();
}

MACHINE_START(S3C2440, "SMDK2440 ported by lihongwqp@163.com")
    /* Maintainer: Ben Dooks <ben-linux@fluff.org> */
    .atag_offset    = 0x100,
    .map_io         = smdk2440_map_io,
    .init_irq       = s3c24xx_init_irq,
    .timer          = &s3c24xx_timer,
    .init_machine   = smdk2440_machine_init,
    .restart        = s3c244x_restart,
MACHINE_END


