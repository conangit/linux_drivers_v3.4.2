#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>


int main(int argc, char* argv[])
{
    static unsigned int cnt = 0;
    int fd[4];
    int key_val = 0;
    int ret;
    int i;
    struct timeval timeout;
    fd_set readfds;
    int mid1, mid2, mid3;
    int maxfdp1;

    fd[0] = open("/dev/button0", O_RDONLY);
    fd[1] = open("/dev/button1", O_RDONLY);
    fd[2] = open("/dev/button2", O_RDONLY);
    fd[3] = open("/dev/button3", O_RDONLY);

    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;   //0.5s

    FD_ZERO(&readfds);

    mid1 = (fd[0] > fd[1]) ? fd[0] : fd[1];
    mid2 = (mid1 > fd[2]) ? mid1 : fd[2];
    mid3 = (mid2 > fd[3]) ? mid2 : fd[3];
    maxfdp1 = mid3 + 1;                     //FD_SETSIZE = 1024

    while(1)
    {
        FD_ZERO(&readfds);

        for (i = 0; i < 4; i++)
            FD_SET(fd[i], &readfds);

        ret = select(maxfdp1, &readfds, NULL, NULL, &timeout);
        if (ret > 0)
        {
            if (FD_ISSET(fd[0], &readfds))
            {
                read(fd[0], &key_val, 4);
                printf("[key0 %d:] val = %d\n", ++cnt, key_val);
            }
            
            if (FD_ISSET(fd[1], &readfds))
            {
                read(fd[1], &key_val, 4);
                printf("[key1 %d:] val = %d\n", ++cnt, key_val);
            }

            if (FD_ISSET(fd[2], &readfds))
            {
                read(fd[2], &key_val, 4);
                printf("[key2 %d:] val = %d\n", ++cnt, key_val);
            }

            if (FD_ISSET(fd[3], &readfds))
            {
                read(fd[3], &key_val, 4);
                printf("[key3 %d:] val = %d\n", ++cnt, key_val);
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

