#ifndef LEDS_CMD_H_
#define LEDS_CMD_H_

/*
 * ioctl-number.txt
 * include\asm-generic\ioctl.h
 */

//  幻数
#define LEDS_MAGIC 'L'

// 命令 = 方向 + size + 幻数 + 序数
#define LEDS_SET _IOW(LEDS_MAGIC, 0x20, unsigned long)
#define LEDS_GET _IOR(LEDS_MAGIC, 0x21, unsigned long)
#define LEDS_ERR _IO(LEDS_MAGIC, 0x22)

#define LEDS_MAXNR 0x22

struct leds_config {
    int data;
    char name[16];
    // char *name;    // 为什么在LEDS_GET返回时候出错? -- 需好好理解C的指针
};

#endif /* end of LEDS_CMD_H_ */