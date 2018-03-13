#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[])
{
    int fd0;
    int fd2;
    int fd3;
    int fd11;
    static unsigned long index = 0;
    int key_val[4] = {1, 1, 1, 1};
    int i;


    fd0 = open("/dev/key-0", O_RDONLY);
    fd2 = open("/dev/key-2", O_RDONLY);
    fd3 = open("/dev/key-3", O_RDONLY);
    fd11 = open("/dev/key-11", O_RDONLY);

    if(fd0 < 0 || fd2 <0 || fd3 <0 || fd11 < 0)
    {
        printf("can't open device!\n");
        return -1;
    }

    while(1)
    {
        read(fd0, &key_val[0], 4);
        read(fd2, &key_val[1], 4);
        read(fd3, &key_val[2], 4);
        read(fd11, &key_val[3], 4);

        for (i = 0; i < 4; i++)
        {
            if (key_val[i] == 0)
            {
                printf("key-%d down.\n", i + 1);
                usleep(500000);
            }
        }

        usleep(10000);
    }

    close(fd0);
    close(fd2);
    close(fd3);
    close(fd11);
    
    return 0;
}


