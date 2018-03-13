#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

int main(int argc, char* argv[])
{
    static unsigned int cnt = 0;
    int fd[4];
    int key_val = 0;
    int ret;
    struct pollfd pfd[4];
    int i;

    fd[0] = open("/dev/button0", O_RDONLY);
    fd[1] = open("/dev/button1", O_RDONLY);
    fd[2] = open("/dev/button2", O_RDONLY);
    fd[3] = open("/dev/button3", O_RDONLY);

    for (i = 0; i < 4; i++)
    {
        pfd[i].fd = fd[i];
        pfd[i].events = POLLIN; 
    }

    while(1)
    {
        ret = poll(pfd, 4, -1);

        if (ret > 0)
        {
            for (i = 0 ; i < 4; i++)
            {
                if (pfd[i].revents & POLLIN)
                {
                    read(pfd[i].fd, &key_val, 4);
                    printf("[%d:] key%d val = %d\n", ++cnt, i, key_val);
                }
            }
        }
    }

    sleep(10);

    for (i = 0; i < 4; i++)
    {
        close(fd[i]);
    }

    printf("close\n");

    return 0;
}

/*
 * 打开kernel debug 可发现 按键按下时候 
 * 多次调用了do_work_func() 最终只有一次调用do_timer_func()
 * 从而实现了按键防抖
 */