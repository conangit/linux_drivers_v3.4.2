#ifndef _DECODE_TERMIOS_H
#define _DECODE_TERMIOS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>


void decode_termios(struct termios *opt);


#endif