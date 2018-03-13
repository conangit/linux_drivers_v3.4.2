#include <stdio.h>

void pk()
{
    __asm__ (
    "ldr r7,=378 \n"
    "swi \n"
    :
    :
    :"memory");
}

int main()
{
    printf("entry sys call\n");
    pk();
    printf("exit sys call\n");

    return 0;
}


