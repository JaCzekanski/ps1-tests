#include <stdio.h>
#include <psxgpu.h>
#include "vram.h"
#include "stdint.h"

DISPENV disp;
DRAWENV draw;

#define SCR_W 320
#define SCR_H 240

void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, w, h);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
}

void initVideo()
{
    ResetGraph(0);
    setResolution(SCR_W, SCR_H);
    SetDispMask(1);
}

void fillRect(int x, int y, int w, int h, int r, int g, int b) {
    FILL f;
    setFill(&f);
    setRGB0(&f, r, g, b);
    setXY0(&f, x, y);
    setWH(&f, w, h);

    DrawPrim(&f);
}

void clearScreen() {
    fillRect(0,   0,   512, 256,   0, 0, 0);
    fillRect(512, 0,   512, 256,   0, 0, 0);
    fillRect(0,   256, 512, 256,   0, 0, 0);
    fillRect(512, 256, 0x3f1, 256, 0, 0, 0);
}

void runTests();

int main() {
    initVideo();
    printf("\ngpu/mask-bit\n");

    setMaskBitSetting(false, false);
    clearScreen();

    runTests();
    
    for (;;) {
        VSync(0);
    }
    return 0;
}
