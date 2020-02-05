#include <common.h>

#define SCR_W 320
#define SCR_H 240

typedef struct CLIP {
	unsigned int tag;
	unsigned int code;
} CLIP;

void setClipping(int left, int top, int right, int bottom) {
    CLIP c;
    setlen(&c, 1);
    c.code = ((top << 10) & 0xffc00) | (left & 0x3ff);
    setcode(&c, 0xe3);
    DrawPrim(&c);

    c.code = (((bottom-1) << 10) & 0xffc00) | ((right-1) & 0x3ff);
    setcode(&c, 0xe4);
    DrawPrim(&c);
}

void drawQuad(int x, int y, int w, int h) {
    POLY_F4 p;
    setPolyF4(&p);
    setRGB0(&p, rand()%255, rand()%255, rand()%255);
    setXY4(&p,
        x,     y,
        x+w, y,
        x,     y+h,
        x+w, y+h
    );
    DrawPrim(&p);
}

void drawRect(int x, int y, int w, int h, int r, int g, int b) {
    TILE p;
    setTile(&p);
    setRGB0(&p, r, g, b);
    setXY0(&p, x, y);
    setWH(&p, w, h);
    DrawPrim(&p);
}

void drawRect(int x, int y, int w, int h) {
    drawRect(x, y, w, h, rand()%255, rand()%255, rand()%255);
}

void setClippingDebug(int left, int top, int right, int bottom) {
    setClipping(0, 0, 1023, 511); // Disable clipping
    drawRect(left, top, right - left, bottom - top, 0xff, 0, 0); // Draw debug outline
    
    setClipping(left, top, right, bottom);
}

int main() {
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/clipping - GP0(0xe3), GP0(0xe4) test\n");
    printf("Rectangle GP0(0x60) and Polygon GP0(0x28) are tested\n");
    printf("You shouldn't see red clipping outlines\n");

    clearScreen();
    fillRect(0, 0, SCR_W, SCR_H, 0xff, 0xff, 0xff);

    const int W = 80;
    const int H = 24;
    int x = 120;
    int y = 40;
    // Top, Left clip, polygon
    setClippingDebug(x + 1, y + 1, x + W, y + H);
    drawQuad(        x,     y,     W,     H);
    
    y += H + 8;
    // Right, Bottom clip, polygon
    setClippingDebug(x, y, x + W - 1, y + H - 1);
    drawQuad(        x, y, W,        H);

    y += H + 8;
    // Top, Left clip, rect
    setClippingDebug(x+ 1, y + 1, x + W, y + H);
    drawRect(        x,    y,     W,     H);
    
    y += H + 8;
    // Right, Bottom clip, rect
    setClippingDebug(x, y, x + W - 1, y + H - 1);
    drawRect(        x, y, W,         H);

    for (;;) {
        VSync(0);
    }
    return 0;
}
