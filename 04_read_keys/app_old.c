#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>


int main(int argc, char* argv[])
{
	int fd;
	unsigned char key_vals[4] = {0};
	
	fd = open("/dev/buttons", O_RDWR);

	if(fd < 0)
	{
		printf("dev can't open\n");
		return 0;
	}

	while(1)
	{
		static unsigned int index = 0;

		read(fd, key_vals, 4);

		if(key_vals[0] == 0)
		{
			index++;
			printf("%d : S2 down\n", index);
		}

		if(key_vals[1] == 0)
		{
			index++;
			printf("%d : S3 down\n", index);
		}

		if(key_vals[2] == 0)
		{
			index++;
			printf("%d : S4 down\n", index);
		}

		if(key_vals[3] == 0)
		{
			index++;
			printf("%d : S5 down\n", index);
		}
	}

	close(fd);
	
	return 0;
}

