#ifndef LEDS_CMD_H_
#define LEDS_CMD_H_

/*
 * ioctl-number.txt
 * include\asm-generic\ioctl.h
 */

#define LEDS_MAGIC 'L'

#define LEDS_SET _IOW(LEDS_MAGIC, 0, int)
#define LEDS_GET _IOR(LEDS_MAGIC, 1, int)
#define LEDS_ERR _IO(LEDS_MAGIC, 2)

#define LEDS_MAXNR 2

struct leds_config {
    int data;
    char *name;
};

#endif /* end of LEDS_CMD_H_ */