#include <common.h>

#define SCR_W 320
#define SCR_H 240

const int texPageX = 640;
const int texPageY = 0;

uint16_t lastTpage = 0;
void setTextureFlip(bool flipY, bool flipX) {
    DR_TPAGE e;
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ 0, getTPage(/* bits = 15bit */ 2, /* transparencyMode */0, texPageX, texPageY));
    e.code[0] |= (flipX << 12);
    e.code[0] |= (flipY << 13);
    lastTpage = e.code[0];
    DrawPrim(&e);
}

void drawRectangle(int x, int y, int w, int h) {
    SPRT s;
    setSprt(&s);
    setSemiTrans(&s, 0);
    setRGB0(&s, 128, 128, 128 );
    s.x0 = x;
    s.y0 = y;
    s.w = w;
    s.h = h;
    s.u0 = 0;
    s.v0 = 0;
    DrawPrim(&s);
}

void drawQuad(int x, int y, int w, int h) {
    POLY_FT4 p;
    setPolyFT4(&p);
    setSemiTrans(&p, 0);
    setRGB0(&p, 128, 128, 128 );
    setClut(&p, 0, 0); // Not used
    p.tpage = lastTpage;

    p.x0 = x;   p.y0 = y;
    p.x1 = x+w; p.y1 = y;
    p.x2 = x;   p.y2 = y+h;
    p.x3 = x+w; p.y3 = y+h;

    p.u0 = 0;   p.v0 = 0;
    p.u1 = w;   p.v1 = 0;
    p.u2 = 0;   p.v2 = h;
    p.u3 = w;   p.v3 = h;
    DrawPrim(&p);
}

int main() {
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/texture-flip\n");
    printf("VRAM contents:\n"
"0,0                                                       \n"
"┌--------------------------------------------------------┐\n"
"|┌--------┐ ┌--------┐     ┌---------┐                   |\n"
"||        | |        |     |         |                   |\n"
"|| Rect   | | Rect   |     | texture |                   |\n"
"|| normal | | x-flip |     |         |                   |\n"
"||        | |        |     |         |                   |\n"
"||        | |        |     |         |                   |\n"
"│└--------┘ └--------┘     └---------┘                   |\n"
"|                                                        |\n"
"│┌--------┐ ┌--------┐     ┌--┐  ┌--┐                    |\n"
"||        | |        |     | r|  | r|  64x64rect         |\n"
"|| Rect   | | Rect   |     └--┘  └--┘  2nd flipped       |\n"
"|| y-flip | | x&y    |                                   |\n"
"||        | | flip   |     ┌--┐  ┌--┐  64x64 quad        |\n"
"||        | |        |     | q|  | q|  2nd flipped       |\n"
"|└--------┘ └--------┘     └--┘  └--┘(should be the same)|\n"
"└--------------------------------------------------------┘\n"
"                                                1023, 1023\n\n");

    clearScreenColor(0xff, 0xff, 0xff);

    const int W = 256;
    const int H = 256;
    // Make texture
    for (int i = 0; i < 0xffff; i++) {
        int x = texPageX + i % W; 
        int y = texPageY + i / W;
        vramPut(x, y, i);
    }

    // 0x0 - normal
    setTextureFlip(0, 0);
    drawRectangle(0, 0, W, H);

    // 260x0 - x flipped
    setTextureFlip(0, 1);
    drawRectangle(260, 0, W, H);

    // 260x0 - y flipped
    setTextureFlip(1, 0);
    drawRectangle(0, 260, W, H);

    // 260x260 - x&&y flipped
    setTextureFlip(1, 1);
    drawRectangle(260, 260, W, H);


    // 640x260 - normal (small rect)
    setTextureFlip(0, 0);
    drawRectangle(640, 260, 64, 64);
    
    // 640x260 - x&&y flipped (small rect)
    setTextureFlip(1, 1);
    drawRectangle(714, 260, 64, 64);


    // Check polygons as well, just to be sure
    // 640x260 - normal (small polygon)
    setTextureFlip(0, 0);
    drawQuad(640, 334, 64, 64);
    
    // 640x260 - x&&y flipped (small polygon)
    setTextureFlip(1, 1);
    drawQuad(714, 334, 64, 64);
    
    printf("Done.\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}
