
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
    static unsigned int cnt = 0;
    int fd = 0;
    unsigned char key_val = 0;
    int ret = 0;

    fd = open("/dev/buttons", O_RDONLY);
    //fd = open("/dev/buttons", O_RDONLY | O_NONBLOCK);

    if (fd < 0)
    {
        printf("device can't open!\n");
        return -1;
    }

    printf("I'am process 2-%d\n", getpid());

    while (1)
    {
        ret = read(fd, &key_val, 1);

        printf("%d : key_val = 0x%x, ret = %d\n", ++cnt, key_val, ret);

        //sleep(3);
    }

    close(fd);

    return 0;
}


