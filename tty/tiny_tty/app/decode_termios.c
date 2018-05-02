#include "decode_termios.h"

void data_bit(struct termios *opt)
{
    switch (opt->c_cflag & CSIZE) {
        case CS5:
            printf("date bit = 5\n");
            break;
        case CS6:
            printf("date bit = 6\n");
            break;
        case CS7:
            printf("date bit = 7\n");
            break;
        default:
        case CS8:
            printf("date bit = 8\n");
            break;
    }
}

void parity(struct termios *opt)
{
    if (opt->c_cflag & PARENB)
    {
        if (opt->c_cflag & PARODD)
            printf("parity = odd\n");
        else
            printf("parity = even\n");
    }
    else
    {
        printf("parity = none\n");
    }
}

void stop_bit(struct termios *opt)
{
    if (opt->c_cflag & CSTOPB)
        printf("stop bit = 2\n");
    else
        printf("stop bit = 1\n");
}

void hw_flow(struct termios *opt)
{
    if (opt->c_cflag & CRTSCTS)
        printf("CRTS/CTS is enabled\n");
    else
        printf("CRTS/CTS is disabled\n");
}

void baud_rate(struct termios *opt)
{
    speed_t ispeed;
    speed_t ospeed;

    speed_t speed_list[] = {B150, B4800, B9600, B38400, B115200, 
        B1000000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000};
    const char* speed_char[] = {"B150", "B4800", "B9600", "B38400", "B115200",
        "B1000000", "B1500000", "B2000000", "B2500000", "B3000000", "B3500000", "B4000000"};

    int i;
    char buf[128];

    ispeed = cfgetispeed(opt);
    ospeed = cfgetospeed(opt);

    // printf("input_speed  = %07o\n", ispeed);
    // printf("output_speed = %07o\n", ospeed);

    for (i = 0; i < (sizeof(speed_list)/sizeof(speed_list[0])); i++)
    {
        if (ispeed == speed_list[i])
        {
            printf("input_speed = %s\n", speed_char[i]);
        }
    }

    for (i = 0; i < (sizeof(speed_list)/sizeof(speed_list[0])); i++)
    {
        if (ospeed == speed_list[i])
        {
            printf("output_speed = %s\n", speed_char[i]);
        }
    }
}

void decode_termios(struct termios *opt)
{
    printf("#######################\n");
    data_bit(opt);
    parity(opt);
    stop_bit(opt);
    baud_rate(opt);
    hw_flow(opt);
    printf("#######################\n");
}


