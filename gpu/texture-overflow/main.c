#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxetc.h>
#include "lena.h"
#include "cube.h"

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

void clearVram() {
    fillRect(0,   0,   512,   256, 0xff, 0x00, 0x00);
    fillRect(512, 0,   512,   256, 0xff, 0x00, 0x00);
    fillRect(0,   256, 512,   256, 0xff, 0x00, 0x00);
    fillRect(512, 256, 0x3f1, 256, 0xff, 0x00, 0x00);
}

void setTexPage(int x, int y) {
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* 15bit */ 2, /* semi-transparency mode */ 0, /*x*/x, /*y*/y);
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ 0, texpage);
    DrawPrim(&e);
}


int main() {
    initVideo();
    printf("\ngpu/texture-overflow\n");

    clearVram();

    TIM_IMAGE tim;
    GetTimInfo((unsigned int*)lena_tim, &tim);
    LoadImage(tim.prect, tim.paddr);
    
    GetTimInfo((unsigned int*)cube_tim, &tim);
    LoadImage(tim.prect, tim.paddr);

    for (;;) {
        fillRect(0, 0, SCR_W, SCR_H, 0xff, 0xff, 0xff);

        setTexPage(896, 256);

        SPRT s;
        setSprt(&s);
        setSemiTrans(&s, 0);
        setRGB0(&s, 128, 128, 128 );
        s.x0 = 16;
        s.y0 = 16;
        s.w = 256;
        s.h = 128;
        s.u0 = 0;
        s.v0 = 0;

        DrawPrim(&s);

        VSync(0);
    }
    return 0;
}
