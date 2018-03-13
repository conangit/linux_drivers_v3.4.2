#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char* argv[])
{
    int fd;
    int val = 1;

    if (argc !=2)
    {
        printf("usage : %s <on |off>\n", argv[0]);
        return 0;
    }
    
    fd = open("/dev/leds", O_RDWR);

    if (fd < 0)
    {
        printf("device can't open\n");
        return -1;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        val = 0;
    }
    else
    {
        val = 1;
    }
    
    write(fd, &val, 4);
    
    close(fd);

    return 0;
}

