#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxgte.h>
#include <psxetc.h>

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
    fillRect(0,   0,   512,   256, 0xff, 0xff, 0xff);
    fillRect(512, 0,   512,   256, 0xff, 0xff, 0xff);
    fillRect(0,   256, 512,   256, 0xff, 0xff, 0xff);
    fillRect(512, 256, 0x3f1, 256, 0xff, 0xff, 0xff);
}

int main() {
    initVideo();
    InitGeom();
    printf("\ngpu/lines\n");

    for (;;) {
        const int PI = 4096/2;
        const int CX = SCR_W/2;
        const int CY = SCR_H/2;
        const int SEGS = 6;
        const int ANGLE = 2 * PI / SEGS;
        const int H_ANGLE = ANGLE / 2;
        const int SCALE = 48;
        
        clearScreen();

        for (int s = 0; s < SEGS; s++) {
            LINE_F2 l;

            setLineF2(&l);
            setRGB0(&l, 0, 0, 0);
            l.x0 = CX + icos(ANGLE * s - H_ANGLE) / SCALE;
            l.y0 = CY + isin(ANGLE * s - H_ANGLE) / SCALE;
            l.x1 = CX + icos(ANGLE * s + H_ANGLE) / SCALE;
            l.y1 = CY + isin(ANGLE * s + H_ANGLE) / SCALE;

            DrawPrim(&l);
        }

        VSync(0);
    }
    return 0;
}
