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
    char buf[128] = {0};
    struct input_event t;
    struct pollfd pfd[4];
    int fd[4];
    int i;
    int ops = 1;

    fd[0] = open("/dev/event0", O_RDONLY);
    fd[1] = open("/dev/event1", O_RDONLY);
    fd[2] = open("/dev/event2", O_RDONLY);
    fd[3] = open("/dev/event3", O_RDONLY);

    if (fd[0] < 0 || fd[1] < 0 || fd[2] < 0 || fd[3] < 0)
    {
        printf("Open device fialed!\n");
        return -1;
    }

    for (i = 0; i < 4; i++)
    {
        pfd[i].fd = fd[i];
        pfd[i].events = POLLIN;
    }

    while (ops)
    {
        ret = poll(pfd, 4, -1);
        if (ret > 0)
        {
            for (i = 0; i < 4; i++)
            {
                if (pfd[i].revents & POLLIN)
                {
                    ret = read(pfd[i].fd, &t, sizeof(t));
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
    }

    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);

    printf("Close device\n");
    
    return 0;
}


