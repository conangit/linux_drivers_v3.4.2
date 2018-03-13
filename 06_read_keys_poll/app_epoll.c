#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
    static unsigned int cnt[4] = {0};
    int fd[4];
    int key_val = 0;
    int ret;
    int i;
    int epfd;
    int nfds;
    struct epoll_event ev[4];

    fd[0] = open("/dev/button0", O_RDONLY);
    fd[1] = open("/dev/button1", O_RDONLY);
    fd[2] = open("/dev/button2", O_RDONLY);
    fd[3] = open("/dev/button3", O_RDONLY);

    epfd = epoll_create(4);

    for (i = 0; i < 4; i++)
    {
        ev[i].events = EPOLLIN;
        ev[i].data.fd = fd[i];
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd[i], &ev[i]);
    }

    while(1)
    {
        //返回需要处理的事件数目
        nfds = epoll_wait(epfd, ev, 4, -1);

        for (i = 0; i < nfds; i++)
        {
            if (ev[i].events & EPOLLIN)
            {
                if (ev[i].data.fd == fd[0])
                {
                    read(fd[0], &key_val, 4);
                    printf("[%d:] key1 = %d\n", ++cnt[0], key_val);
                }
                
                if (ev[i].data.fd == fd[1])
                {
                    read(fd[1], &key_val, 4);
                    printf("[%d:] key2 = %d\n", ++cnt[1], key_val);
                }
                
                if (ev[i].data.fd == fd[2])
                {
                    read(fd[2], &key_val, 4);
                    printf("[%d:] key3 = %d\n", ++cnt[2], key_val);
                }
                
                if (ev[i].data.fd == fd[3])
                {
                    read(fd[3], &key_val, 4);
                    printf("[%d:] key4 = %d\n", ++cnt[3], key_val);
                }
            }
        }
    }

    sleep(10);

    for (i = 0; i < 4; i++)
    {
        close(fd[i]);
    }

    close(epfd);

    printf("close\n");

    return 0;
}

