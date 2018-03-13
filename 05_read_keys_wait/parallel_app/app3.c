#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[])
{
    static unsigned int cnt = 0;
    int fd;
    int key_val = 0;
    int ret;


    fd = open("/dev/button2", O_RDONLY);

    while(1)
    {
        ret = read(fd, &key_val, 4);

        if (ret)
        {
           printf("[key3 %d:] val = %d\n", ++cnt, key_val);
        }
    }

    sleep(10);

    close(fd);

    printf("close\n");

    return 0;
}

