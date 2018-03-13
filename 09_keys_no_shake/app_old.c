/*7th*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

int fd = 0;

static void signal_handler(int signum)
{
    unsigned char key_val = 0;
    static unsigned int cnt = 0;
    
    read(fd, &key_val, 1);

    printf("[%d]: signum = %d, key_val = 0x%x\n", ++cnt, signum, key_val);
}

int main(int argc, char* argv[])
{
    int oflags = 0;
    
    fd = open("/dev/buttons", O_RDWR);

    if(fd < 0)
    {
        printf("dev can't open\n");
        return -1;
    }

    signal(SIGIO, signal_handler);

    fcntl(fd, F_SETOWN, getpid());
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags|FASYNC);
    
    while(1)
    {
        sleep(5);
    }

    close(fd);

    return 0;
}


