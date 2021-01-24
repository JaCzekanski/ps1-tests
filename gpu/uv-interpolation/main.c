#include <common.h>
#include <psxgpu.h>

#define SCR_W 320
#define SCR_H 240

void setE1(int transparencyMode, int dithering) {
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* bitcount - do not care */0, /* semi-transparency mode */ transparencyMode, /*x*/0, /*y*/0);
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ dithering, texpage);
    DrawPrim(&e);
}


const uint16_t u = 512;
const uint16_t v = 0;

void interpolateUv(int X, int Y) {
    for (int i = 0; i<256; i++) {
        int x = X;
        int y = Y+i;
        int w = i;

        POLY_FT4 p;
        setPolyFT4(&p);
        setRGB0(&p, 0x80, 0x80, 0x80);
        setXY4(&p,
            x,   y,
            x+w, y,
            x,   y+1,
            x+w, y+1
        );
        setUV4(&p,
            u,   v,
            u+1, v,
            u,   v,
            u+1, v
        );
        setTPage(&p, /* 15bit */ 2, /* b/2+f/2 */ 0, u, v);
        DrawPrim(&p);
    }
}
void interpolateColor(int X, int Y) {
    for (int i = 0; i<256; i++) {
        int x = X;
        int y = Y + i;
        int w = i;

        POLY_G4 p;
        setPolyG4(&p);
        setRGB0(&p, 0xff, 0x00, 0x00);
        setRGB1(&p, 0x00, 0xff, 0x00);
        setRGB2(&p, 0xff, 0x00, 0x00);
        setRGB3(&p, 0x00, 0xff, 0x00);
        setXY4(&p,
            x,   y,
            x+w, y,
            x,   y+1,
            x+w, y+1
        );
        DrawPrim(&p);
    }
}

int main()
{
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/uv-interpolation\n");

    clearScreen();
    setE1(0, 0);

    vramPut(u+0, v, 0x001f); // full red
    vramPut(u+1, v, 0x03E0); // full green

    interpolateUv(0, 0);
    interpolateColor(0, 256);
    
    setE1(0, 1);
    interpolateColor(256, 256);

    for (;;) {
        VSync(0);
    }
    return 0;
}
