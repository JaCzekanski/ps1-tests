#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxetc.h>
#include "io.h"

DISPENV disp;
DRAWENV draw;

void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, w, h);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
}

void initVideo()
{
    ResetGraph(0);
    setResolution(320, 240);
    SetDispMask(1);
}

void __attribute__((optimize("O1"))) delay(uint32_t cycles)
{
    uint32_t delay = cycles >> 2;
    do {
        __asm__ __volatile__ ("nop");
        --delay;
    } while (delay != 0);
}

int main()
{
    printf("spu/test\n");
    initVideo();

    printf("16 bit access\n");
    for (int try = 0; try < 32; try++) {
      for (int i = 0; i<16; i++) {
        uint16_t v = 1 << i;
        write16(0x1F801D80, v);
        
        delay(10000);

        uint16_t r = read16(0x1F801D80);

        if (v != r) {
          printf("0x%04x written, but read 0x%04x\n", v, r);
          break;
        }
      }
    }
    
    printf("Done\n");

    printf("32 bit access\n");
    for (int try = 0; try < 32; try++) {
      for (int i = 0; i<32; i++) {
        uint32_t v = 1 << i;
        write32(0x1F801D80, v);
        
        delay(10000);

        uint32_t r = read32(0x1F801D80);

        if (v != r) {
          printf("0x%08x written, but read 0x%08x\n", v, r);
          break;
        }
      }
    }

    printf("\n\nDone, crashing now...\n");
    __asm__ volatile (".word 0xFC000000"); // Invalid opcode (63)
    return 0;
}
