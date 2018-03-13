#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[])
{
    int fd;
    unsigned char key_val = 0;
    
    fd = open("/dev/buttons", O_RDONLY);
    //fd = open("/dev/buttons", O_RDONLY | O_NONBLOCK);

    if(fd < 0)
    {
        printf("dev can't open\n");
        return 0;
    }

    while(1)
    {
        static unsigned int cnt = 0;
        
        read(fd, &key_val, 1);
        printf("%d : key_val = 0x%x\n", ++cnt, key_val);
    }

    close(fd);

    return 0;
}

