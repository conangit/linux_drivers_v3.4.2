#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include "decode_termios.h"
speed_t speed_list[12] = {B150, B4800, B9600, B38400, B115200, 
    B1000000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000};

#define DEV "/dev/ttyPL5"

static int fd;

static void write_ops(fd)
{
    char wr_buf[] = "abcdefghijklmnQ";  //字符加'\0' = 16
    int size;
    int ret1, ret2;

    size = sizeof(wr_buf) / sizeof(wr_buf[0]);
    printf("buf size = %d\n", size);
    sleep(1);
    
    ret1 = write(fd, wr_buf, 10);
    ret2 = write(fd, wr_buf, 15);
    printf("write ret1 = %d, ret2 = %d\n", ret1, ret2);
}

static void read_ops(fd)
{
    char rd_buf[128] = {0};
    int retval;
    static unsigned int k = 0;

#if 0
    retval = read(fd, rd_buf, 128);
    printf("read retval = %d\n", retval);
    printf("read data: %s\n", rd_buf);
#else
    while (1)
    {
        retval = read(fd, rd_buf, 128);
        if (retval > 0) 
        {
            printf("[%lu]\n", ++k);
            printf("read retval = %d\n", retval);
            printf("read data: %s\n", rd_buf);
            sleep(0.5);
            memset(rd_buf, 0 , 128);
            sleep(0.5);
        }
    }
#endif
}

static void set_ops(fd)
{
    struct termios opt;

    tcgetattr(fd, &opt);
    decode_termios(&opt);

    // 比不可少
    cfmakeraw(&opt);

    #define BUNDRATE (speed_list[4])
    cfsetispeed(&opt, BUNDRATE);
    cfsetospeed(&opt, BUNDRATE);

    // data_bits 5 6 7 8
    opt.c_cflag &= ~CSIZE;
    // opt.c_cflag |= CS5;
    // opt.c_cflag |= CS6;
    // opt.c_cflag |= CS7;
    opt.c_cflag |= CS8;

    // parity
    // opt.c_cflag |= PARENB;            // even
    // opt.c_cflag |= (PARENB | PARODD); // odd

    // stop 1 or 2
    // opt.c_cflag |= CSTOPB;  // 2
    
    tcsetattr(fd, TCSANOW, &opt);
    decode_termios(&opt);
}

int main(void)
{
    // open
    printf("******************************************\n");
    fd = open(DEV, O_RDWR);
    if (fd < 0)
    {
        printf("Error: cann't open device: %d\n", fd);
        return -1;
    }
    printf("Open device %s, fd = %d\n", DEV, fd);
    printf("##########################################\n");

    // config
    printf("******************************************\n");
    set_ops(fd);
    printf("##########################################\n");
    sleep(1);

    // write
    // 简单验证方法
    // echo abcdefg > /dev/ttyPL0
    printf("******************************************\n");
    write_ops(fd);
    printf("##########################################\n");
    sleep(1);

    // read
    // printf("******************************************\n");
    // read_ops(fd);
    // printf("##########################################\n");
    // sleep(1);
     
    // close
    printf("******************************************\n");
    close(fd);
    printf("close device %s\n", DEV);
    printf("##########################################\n");


    return 0;
}


