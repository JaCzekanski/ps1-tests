#include "delay.h"

void __attribute__((optimize("O1"))) delay(uint32_t cycles)
{
    uint32_t delay = cycles >> 2;
    do {
        __asm__ __volatile__ ("nop");
        --delay;
    } while (delay != 0);
}