#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "leds_cmd.h"

/*
 * int ioctl(int fd, unsigned long request, ...);
 */

int main(int argc, char* argv[])
{
    int fd1, fd2, fd3;
    int ret;
    struct leds_config msg;

    fd1 = open("/dev/led4", O_RDWR);
    fd2 = open("/dev/led5", O_RDWR);
    fd3 = open("/dev/led6", O_RDWR);

    memset(&msg, 0, sizeof(struct leds_config));

    // ioctl write
    msg.data = 1;
    msg.name = "led4";
    ret = ioctl(fd1, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);
    
    msg.data = 0;
    ret = ioctl(fd1, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);
    
    msg.data = 1;
    msg.name = "led5";
    ret = ioctl(fd2, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);

    msg.data = 1;
    msg.name = "led6";
    ret = ioctl(fd3, LEDS_SET, &msg);
    printf("app:[LEDS_SET] ret = %d\n", ret);
    sleep(1);

    // ioctl read
    ret = ioctl(fd1, LEDS_GET, &msg);
    printf("app:[LEDS_GET] ret = %d, %s = %d\n", ret, msg.name, msg.data);
    sleep(1);

    ret = ioctl(fd3, LEDS_GET, &msg);
    printf("app:[LEDS_GET] ret = %d, %s = %d\n", ret, msg.name, msg.data);
    sleep(1);

    close(fd1);
    close(fd2);
    close(fd3);

    return 0;
}


