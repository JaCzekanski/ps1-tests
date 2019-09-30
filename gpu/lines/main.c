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

void circle(int cx, int cy, int scale, int segs) {
    const int PI = 4096/2;

    LINE_F2 l;

    setLineF2(&l);
    setRGB0(&l, 0, 0, 0);

    const int ANGLE = 2 * PI / segs;
    const int H_ANGLE = ANGLE / 2;
    for (int s = 0; s < segs; s++) {
        l.x0 = cx + icos(ANGLE * s - H_ANGLE) / scale;
        l.y0 = cy + isin(ANGLE * s - H_ANGLE) / scale;
        l.x1 = cx + icos(ANGLE * s + H_ANGLE) / scale;
        l.y1 = cy + isin(ANGLE * s + H_ANGLE) / scale;

        DrawPrim(&l);
    }
}

int main() {
    initVideo();
    InitGeom();
    printf("\ngpu/lines\n");

    for (;;) {
        
        clearScreen();
        
        LINE_F2 l;

        setLineF2(&l);
        setRGB0(&l, 0, 0, 0);

        // Horizontal lines
        for (int y = 0; y < 10; y++) {
            l.x0 = 16;
            l.y0 = 16 + y * 8;
            l.x1 = 16 + 80;
            l.y1 = 16 + y * 8 + y;

            DrawPrim(&l);
        }
        
        // Vertical lines
        for (int x = 0; x < 10; x++) {
            l.x0 = 108 + x * 8;
            l.y0 = 16;
            l.x1 = 108 + x * 8 + x;
            l.y1 = 16 + 80;
            
            DrawPrim(&l);
        }
        
        // Horizontal lines
        for (int y = 0; y < 40; y++) {
            l.x0 = 200;
            l.y0 = 16 + y * 4;
            l.x1 = 200 + 40 - y;
            l.y1 = 17 + y * 4;

            DrawPrim(&l);
        }
        

        circle(SCR_W * 1/4, SCR_H * 4/6, 100, 6);

        VSync(0);
    }
    return 0;
}
