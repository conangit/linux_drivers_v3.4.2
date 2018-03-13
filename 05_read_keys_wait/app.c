#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[])
{
    static unsigned int cnt = 0;
    int fd0, fd1, fd2, fd3;
    int key_val[4] = {0};
    int ret0 = 0;
    int ret1 = 0;
    int ret2 = 0;
    int ret3 = 0;

    fd0 = open("/dev/button0", O_RDONLY);
    //fd1 = open("/dev/button1", O_RDONLY);
    //fd2 = open("/dev/button2", O_RDONLY);
    //fd3 = open("/dev/button3", O_RDONLY);

    while(1)
    {
        ret0 = read(fd0, &key_val[0], 4);
        //ret1 = read(fd1, &key_val[1], 4);
        //ret2 = read(fd2, &key_val[2], 4);
        //ret3 = read(fd3, &key_val[3], 4);

        if (ret0 || ret1 || ret2 || ret3)
        {
           printf("[%d:] key_val = %d %d %d %d\n", ++cnt, key_val[0], key_val[1], key_val[2], key_val[3]);
        }
    }

    sleep(10);

    close(fd0);
    //close(fd1);
    //close(fd2);
    //close(fd3);

    printf("close\n");

    return 0;
}

