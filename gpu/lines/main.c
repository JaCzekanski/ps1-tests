#include <common.h>
#include <psxgte.h>
#include <psxetc.h>

#define SCR_W 320
#define SCR_H 240

void setE1(int transparencyMode, int dithering) {
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* bitcount - do not care */0, /* semi-transparency mode */ transparencyMode, /*x*/0, /*y*/0);
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */  dithering, texpage);
    DrawPrim(&e);
}

void horizontalLines(int startX, int startY) {
    for (int y = 0; y < 10; y++) {
        LINE_F2 l;
        setLineF2(&l);
        setRGB0(&l, 0, 0, 0);

        l.x0 = startX;
        l.y0 = startY + y * 8;
        l.x1 = startX + 80;
        l.y1 = startY + y * 8 + y;

        DrawPrim(&l);
    }
}

void verticalLines(int startX, int startY) {
    for (int x = 0; x < 10; x++) {
        LINE_F2 l;
        setLineF2(&l);
        setRGB0(&l, 0, 0, 0);

        l.x0 = startX + x * 8;
        l.y0 = startY;
        l.x1 = startX + x * 8 + x;
        l.y1 = startY + 80;
        
        DrawPrim(&l);
    }
}

void horizontalLines2(int startX, int startY) {
    for (int y = 0; y < 20; y++) {
        LINE_F2 l;
        setLineF2(&l);
        setRGB0(&l, 0, 0, 0);

        l.x0 = startX;
        l.y0 = startY + y * 4;
        l.x1 = startX + y;
        l.y1 = startY + y * 4 + 1;

        DrawPrim(&l);
    }
}

void flatLines(int startX, int startY)  {
    for (int i = 0; i < 64; i++) {
        LINE_F2 l;
        setLineF2(&l);
        setRGB0(&l, 0xaa, 0x00, 0x00);

        l.x0 = startX;
        l.y0 = startY + i;
        l.x1 = startX + i;
        l.y1 = startY + i;

        DrawPrim(&l);
    }
}

void gouraudLines(int startX, int startY)  {
    for (int i = 0; i < 64; i++) {
        LINE_G2 l;
        setLineG2(&l);
        setRGB0(&l, 0x00, 0x00, 0x00);
        setRGB1(&l, 0xff, 0x00, 0x00);

        l.x0 = startX;
        l.y0 = startY + i;
        l.x1 = startX + i;
        l.y1 = startY + i;

        DrawPrim(&l);
    }
}

void flatMultiLine(int startX, int startY, bool transparent)  {
    LINE_F4 l;
    setLineF4(&l);
    setSemiTrans(&l, transparent);

    setRGB0(&l, 0xaa, 0, 0);

    l.x0 = startX;
    l.y0 = startY;

    l.x1 = startX + 32;
    l.y1 = startY;

    l.x2 = startX + 32;
    l.y2 = startY + 32;

    l.x3 = startX;
    l.y3 = startY;

    DrawPrim(&l);
}

void gouraudMultiLine(int startX, int startY, bool transparent)  {
    LINE_G4 l;
    setLineG4(&l);
    setSemiTrans(&l, transparent);

    setRGB0(&l, 0xff, 0x00, 0x00);
    setRGB1(&l, 0x00, 0xff, 0x00);
    setRGB2(&l, 0x00, 0x00, 0xff);
    setRGB2(&l, 0x00, 0xff, 0xff);

    l.x0 = startX;
    l.y0 = startY;

    l.x1 = startX + 32;
    l.y1 = startY;

    l.x2 = startX + 32;
    l.y2 = startY + 32;

    l.x3 = startX;
    l.y3 = startY;

    DrawPrim(&l);
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
    initVideo(SCR_W, SCR_H);
    InitGeom();
    printf("\ngpu/lines\n");

    for (;;) {
        clearScreenColor(0xff, 0xff, 0xff);

        // Dithering off
        setE1(0, 0); 

        horizontalLines(16, 16);
        verticalLines(110, 16);
        horizontalLines2(200, 16);

        setE1(0, 0); 
        flatLines(16, 100);
        gouraudLines(16, 166);
        
        // Dithering on
        setE1(0, 1); 
        
        flatLines(84, 100);
        gouraudLines(84, 166);
        
        flatMultiLine(150, 100, false); // Opaque 
        flatMultiLine(210, 100, true);  // Transparent 

        gouraudMultiLine(150, 140, false); // Opaque 
        gouraudMultiLine(210, 140, true);  // Transparent 

        circle(200, 200, 140, 16);

        VSync(0);
    }
    return 0;
}
