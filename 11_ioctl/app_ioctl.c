#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "leds_cmd.h"

/*
 * int ioctl(int fd, unsigned long cmd, ...);
 */

int main(int argc, char* argv[])
{
    int fd1, fd2, fd3;
    int ret;
    struct leds_config msg;

    fd1 = open("/dev/led4", O_RDWR);
    fd2 = open("/dev/led5", O_RDWR);
    fd3 = open("/dev/led6", O_RDWR);

    // printf("sizeof(struct leds_config) = %d\n", sizeof(struct leds_config));
    // msg.name = malloc(16);
    // printf("sizeof(struct leds_config) = %d\n", sizeof(struct leds_config));
    memset(&msg, 0, sizeof(struct leds_config));

#if 1
    // ioctl write
    msg.data = 1;
    // msg.name = "led4";
    strcpy(msg.name, "led4");
    ret = ioctl(fd1, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);
    
    msg.data = 0;
    ret = ioctl(fd1, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);
    
    msg.data = 1;
    // msg.name = "led5";
    strcpy(msg.name, "led5");
    ret = ioctl(fd2, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);

    msg.data = 1;
    // msg.name = "led6";
    strcpy(msg.name, "led6");
    ret = ioctl(fd3, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);
#endif

    // ioctl read
    ret = ioctl(fd1, LEDS_GET, &msg);
    printf("app:[LEDS_GET] ret = %d, name = %s, data = %d\n", ret, msg.name, msg.data);
    sleep(1);
    
    ret = ioctl(fd2, LEDS_GET, &msg);
    printf("app:[LEDS_GET] ret = %d, name = %s, data = %d\n", ret, msg.name, msg.data);
    sleep(1);

    ret = ioctl(fd3, LEDS_GET, &msg);
    printf("app:[LEDS_GET] ret = %d, name = %s, data = %d\n", ret, msg.name, msg.data);
    sleep(1);

    // driver未实现命令
    ret = ioctl(fd1, LEDS_ERR);
    printf("app:[LEDS_ERR] ret = %d\n", ret);

    close(fd1);
    close(fd2);
    close(fd3);

    return 0;
}


