#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>

DISPENV disp;
DRAWENV draw;

#define SCR_W 320
#define SCR_H 240

#define SQRT3_2 0.866025f

void setResolution(int w, int h)
{
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, 1024, 512);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
}

void initVideo()
{
    ResetGraph(0);
    setResolution(SCR_W, SCR_H);
    SetDispMask(1);
}

void clearScreen()
{
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

void drawTriangle(int cx, int cy, int a)
{
    float h = a * SQRT3_2;

    int x[3], y[3];
    x[0] = cx - a / 2; y[0] = cy + h / 2;
    x[1] = cx + a / 2; y[1] = cy + h / 2;
    x[2] = cx;         y[2] = cy - h / 2;

    POLY_G3 p;
    setPolyG3(&p);
    setRGB0(&p, 255, 0, 0);
    setRGB1(&p, 0, 255, 0);
    setRGB2(&p, 0, 0, 255);
    setXY3(&p,
        x[0], y[0],
        x[1], y[1],
        x[2], y[2]
    );
    DrawPrim(&p);
}

void setDithering(int dithering) 
{
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* bitcount - do not care */0, /* semi-transparency mode */ 0, /*x*/0, /*y*/0);
    setDrawTPage(&e, /* Drawing to display area */ 1, dithering, texpage);
    DrawPrim(&e);
}

int main()
{
    initVideo();
    printf("\ngpu/triangle (shading test)\n");

    for (;;)
    {
        clearScreen();

        setDithering(0);
        drawTriangle(SCR_W/2, SCR_H/2, 240);        
        drawTriangle(768, 256, 500);

        setDithering(1);
        drawTriangle(SCR_W/2, SCR_H + SCR_H/2, 240);      

        VSync(0);
    }
    return 0;
}
