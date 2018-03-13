#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>


int fd;

void signal_handler(int signum)
{
    unsigned char key_val = 0;
    static unsigned int cnt = 0;

    read(fd, &key_val, 1);

    printf("[key3 %d:] val = 0x%x\n", ++cnt, key_val);
}

int main(int argc, char* argv[])
{
    int oflags = 0;

    fd = open("/dev/button2", O_RDONLY);

    signal(SIGIO, signal_handler);

    //设置filp->f_owner 让内核知道通知哪个进程
    fcntl(fd, F_SETOWN, getpid());

    //设置FASYNC标志
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC);
    while(1)
    {
        sleep(1000);
    }

    close(fd);

    return 0;
}


