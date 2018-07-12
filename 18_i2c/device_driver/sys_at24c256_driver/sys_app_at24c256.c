#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


static int i2c_test(char *dev)
{
    int fd;
    char w_data[256];
    char r_data[256];
    int i;

    fd = open(dev, O_RDWR);
    if (fd < 0)
    {
        printf("Open %s failed\n", dev);
        return -1;
    }
    printf("Open %s...\n", dev);

    //写入数据
    for (i = 0; i < 256; i++)
        w_data[i] = i;

    lseek(fd, 0, SEEK_SET);
    write(fd, w_data, 256);
    
    //读出数据
    lseek(fd, 0, SEEK_SET);
    read(fd, r_data, 256);

    for (i = 0; i < 256; i++)
    {
        if (i > 0 && i%16 == 0)
            printf("\n");
        printf("%4d ", r_data[i]);
    }
    printf("\n");

    return close(fd);
}


int main(void)
{
    int i;
    int ret;
    int k = 0;
    
    char *tmp_dev1 = "/sys/bus/i2c/devices/";
    char *tmp_dev2 = "-0050/eeprom";
    char dev[128] = {0};
    int id = 0;
   
    printf("Input i2c bus id:");
    scanf("%d", &id);
    sprintf(dev, "%s%d%s", tmp_dev1, id, tmp_dev2);
    printf("EEPROM: %s\n", dev);

    for (i = 0; i < 1; i++)
    {
        printf("test cnt:[%d]\n", i);
        sleep(1);
        ret = i2c_test(dev);

        if (ret != 0)
            k++;
    }

    printf("test error: %d\n", k);

    return 0;
}



