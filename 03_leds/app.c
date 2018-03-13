#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int fd0 = 0;
int fd1 = 0;
int fd2 = 0;

void read_led(int index)
{
    int fd;
    int status;
    ssize_t size;

    switch (index)
    {
        case 0:
            fd = fd0;
            break;
        case 1:
            fd = fd1;
            break;
        case 2:
            fd = fd2;
            break;
        default:
            break;
    }
    
    size = read(fd, &status, 4);
    //printf("read size = %d\n", size);
    if (status == 1)
        printf("led-%d on\n", index);
    if (status == 0)
        printf("led-%d off\n", index);
}

void write_led(int index, int val)
{
    int fd;
    int status;
    ssize_t size;

    switch (index)
    {
        case 0:
            fd = fd0;
            break;
        case 1:
            fd = fd1;
            break;
        case 2:
            fd = fd2;
            break;
        default:
            break;
    }

    size = write(fd, &val, 4);
    //printf("write size = %d\n", size);
}


int main(int argc, char* argv[])
{
    int no = -1;
    int status = -1;

#if 0
    fd0 = open("/dev/led-0", O_RDWR);
    fd1 = open("/dev/led-1", O_RDWR);
    fd2 = open("/dev/led-2", O_RDWR);
#endif

#if 1
    if (argc != 3)
    {
        printf("usage :%s <0|1|2> <on|off>\n", argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "0") == 0)
    {
        no = 0;
        fd0 = open("/dev/led-0", O_RDWR);
    }
    else if (strcmp(argv[1], "1") == 0)
    {
        no = 1;
        fd1 = open("/dev/led-1", O_RDWR);
    }
    else if (strcmp(argv[1], "2") == 0)
    {
        no = 2;
        fd2 = open("/dev/led-2", O_RDWR);
    }

    if (strcmp(argv[2], "on") == 0)
    {
        status = 1;
    }
    else if (strcmp(argv[2], "off") == 0)
    {
        status = 0;
    }

    if (fd0 < 0 || fd1 < 0 || fd2 < 0)
    {
        printf("device can't open\n");
        return -1;
    }

    if  ( (no == 0 || no == 1 || no == 2) && (status == 0 || status == 1) )
    {
        //printf("no = %d, status = %d\n", no, status);
        write_led(no, status);
        read_led(no);
    }
#endif


#if 0
    read_led(0);
    read_led(1);
    read_led(2);

    write_led(0, 1);
    write_led(1, 1);
    write_led(2, 1);

    read_led(0);
    read_led(1);
    read_led(2);

    //write_led(0, 0);
    write_led(1, 0);
    //write_led(0, 0);

    read_led(0);
    read_led(1);
    read_led(2);
#endif

#if 0
    read_led(0);
    read_led(1);
    read_led(2);

    write_led(0, 1);
    read_led(0);
    
    write_led(1, 1);
    read_led(1);
    
    write_led(2, 1);
    read_led(2);

    read_led(0);
    read_led(1);
    read_led(2);
#endif


    close(fd0);
    close(fd1);
    close(fd2);

    return 0;
}

