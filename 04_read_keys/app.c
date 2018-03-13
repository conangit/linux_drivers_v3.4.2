#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[])
{
    int fd = 0;
    int key_no = -1;
    static unsigned long index = 0;
    int key_val = 1;

    if (argc != 2)
    {
        printf("usage :%s <0|2|3|11>\n", argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "0") == 0)
    {
        key_no = 0;
        fd = open("/dev/key-0", O_RDONLY);
    }
    else if (strcmp(argv[1], "2") == 0)
    {
        key_no = 2;
        fd = open("/dev/key-2", O_RDONLY);
    }
    else if (strcmp(argv[1], "3") == 0)
    {
        key_no = 3;
        fd = open("/dev/key-3", O_RDONLY);
    }
    else if (strcmp(argv[1], "11") == 0)
    {
        key_no = 11;
        fd = open("/dev/key-11", O_RDONLY);
    }

    if(fd < 0)
    {
        printf("can't open device!\n");
        return -1;
    }

    while(1)
    {
        read(fd, &key_val, 4);

        if (key_val == 0)
        {
            printf("key down %d\n", index++);
            usleep(500000);
        }

        usleep(10000);
    }

    close(fd);
    
    return 0;
}


