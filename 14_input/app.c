#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <poll.h>
#include <linux/input.h>


int main(int argc, char* argv[])
{
    int ret;
    struct input_event t;
    struct pollfd pfd;
    int fd;
    int ops = 1;

    fd = open("/dev/event0", O_RDONLY);

    if (fd < 0)
    {
        printf("Open device fialed!\n");
        return -1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;

    while (ops)
    {
        ret = poll(&pfd, 1, -1);
        if (ret > 0)
        {
            if (pfd.revents & POLLIN)
            {
                ret = read(pfd.fd, &t, sizeof(t));
                if (ret)
                {
                    if (t.code == KEY_ESC)
                    {
                        printf("ESC\n");
                        ops = 0;
                    }
            
                    if ((t.type == EV_KEY) && ops)
                        printf("code = %u, key_vale = %d\n", t.code, t.value);
                }
            }
            
        }
    }

    close(fd);

    printf("Close device\n");
    
    return 0;
}


