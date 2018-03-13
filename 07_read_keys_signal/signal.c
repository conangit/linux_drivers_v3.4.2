#include <stdio.h>
#include <signal.h>

void my_signal_func(int signum)
{
    static unsigned int cnt = 0;

    printf("%d : signum = %d\n", ++cnt, signum);
}

int main(int argc, char *argv[])
{
    signal(SIGUSR1, my_signal_func);        //SIGUSR1 = 10

    while(1)
    {
        sleep(1000);
    }

    return 0;
}


