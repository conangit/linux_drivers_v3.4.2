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

#define DEV "/dev/ttyPL1"
// #define DEV "/dev/ttySL0"

static int fd;

void write_ops(void)
{
    char wr_buf[] = "abcdefghijklnmQ";
    int size;
    int retval;

    size = sizeof(wr_buf) / sizeof(wr_buf[0]);
    printf("write size = %d\n", size);
    sleep(1);
    
    retval = write(fd, wr_buf, size);
    printf("write retval = %d\n", retval);
}

void read_ops(void)
{
    struct termios opt;
    char rd_buf[128] = {0};
    int retval;
    int i;

    tcgetattr(fd, &opt);

    decode_termios(&opt);
    
    cfmakeraw(&opt);

#define BUNDRATE (speed_list[4])
    cfsetispeed(&opt, BUNDRATE);
    cfsetospeed(&opt, BUNDRATE);
    
    tcsetattr(fd, TCSANOW, &opt);

    decode_termios(&opt);

    retval = read(fd, rd_buf, 128);
    printf("read retval = %d\n", retval);
    printf("read data:");
    for(i = 0; rd_buf[i] != '\0'; i++)
        printf("%c ", rd_buf[i]);
    printf("\n");
}

int main(void)
{
    fd = open(DEV, O_RDWR);
    // fd = open("/dev/ttyPS1", O_RDWR);
    // fd = open("/dev/ttyO0", O_RDWR);
    if (fd < 0)
    {
        printf("Error: cann't open device: %d\n", fd);
        return -1;
    }

    printf("Open device %s, fd = %d\n", DEV, fd);

    write_ops();
    sleep(0.5);

    read_ops();
    
    // sleep(0.5);
    // write_ops();

    close(fd);
    printf("close device %s\n", DEV);

    return 0;
}


