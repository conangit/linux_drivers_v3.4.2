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
    int fd;
    unsigned char key_val = 0;
    int ret = 0;

    fd = open("/dev/buttons", O_RDONLY | O_NONBLOCK);

    if(fd < 0)
    {
        printf("device can't open\n");
        return -1;
    }

    struct pollfd fds = {
        .fd     = fd,
        .events = POLLIN,
    };

    while(1)
    {
        ret = poll(&fds, 1, 5000);          //5s

        if (0 == ret)
        {
            printf("timeout\n");
        }
        else if (ret > 0)
        {
            read(fd, &key_val, 1);
        
            printf("[%d]: key_val = 0x%x\n", ++cnt, key_val);
        }
        else if (-1 == ret)
        {
            printf("error!\n");
        }
    }

    close(fd);

    return 0;
}


