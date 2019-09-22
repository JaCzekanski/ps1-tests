#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
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

void clearScreen() {
    FILL f;
    setFill(&f);
    setRGB0(&f, 0xff, 0xff, 0xff);
    setXY0(&f, 0, 0);
    setWH(&f, 1023, 256);

    DrawPrim(&f);
    
    setXY0(&f, 0, 256);
    setWH(&f, 1023, 256);

    DrawPrim(&f);
}

POLY_F4 poly;

int main()
{
    initVideo();
    printf("\ngpu/quad (semi-transparent seam test)\n");

    clearScreen();

    PutDrawEnv(&draw);

    // Draw semi-transparent polygon
    POLY_F4 *p = &poly;
    setPolyF4(p);
    setSemiTrans(p, 1);

    int x[4], y[4];
    x[0] = 48;  y[0] = 48;
    x[1] = 176; y[1] = 32;
    x[2] = 64;  y[2] = 144;
    x[3] = 208; y[3] = 160;

    // Center
    setRGB0(p, 0, 0, 0);
    setXY4(p,
        x[0], y[0],
        x[1], y[1],
        x[2], y[2],
        x[3], y[3]
    );
    DrawPrim(p);

    // Left
    setRGB0(p, 0xff, 0, 0);
    setXY4(p,
        0, 0,
        x[0], y[0],
        0, SCR_H,
        x[2], y[2]
    );
    DrawPrim(p);
    
    // TOP
    setRGB0(p, 0, 0xff, 0);
    setXY4(p,
        0, 0,
        SCR_W, 0,
        x[0], 48,
        x[1], y[1]
    );
    DrawPrim(p);
    
    // RIGHT
    setRGB0(p, 0, 0, 0xff);
    setXY4(p,
        x[1], y[1],
        SCR_W, 0,
        x[3], y[3],
        SCR_W, SCR_H
    );
    DrawPrim(p);
    
    // BOTTOM
    setRGB0(p, 0xff, 0, 0xff);
    setXY4(p,
        x[2], y[2],
        x[3], y[3],
        0, SCR_H,
        SCR_W, SCR_H
    );
    DrawPrim(p);


    // Draw few transparent polygons next to each other
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 8; x++) {
            const int S = 16;
            unsigned char c = (y == 0) ? 0xa0 : 0xff;
            setRGB0(p, c * ((x>>0)&1), c * ((x>>1)&1), c * ((x>>2)&1));
            setXY4(p,
                64 + S*x, 192 + S*y,
                64 + S*x + S, 192 + S*y,
                64 + S*x, 192 + S + S*y,
                64 + S*x + S, 192 + S + S*y
            );
            DrawPrim(p);
        }
    }

    for (;;) {
        VSync(0);
    }
    printf("Done, crashing now...\n");
    __asm__ volatile (".word 0xFC000000"); // Invalid opcode (63)
    return 0;
}
