#include <common.h>

#define SCR_W 320
#define SCR_H 240

void setSemiTransparencyMode(int mode) {
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* bitcount - do not card */0, /* semi-transparency mode */ mode, /*x*/0, /*y*/0);
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ 0, texpage);
    DrawPrim(&e);
}

const unsigned char bgColors[4] = {
    0, 64, 128, 255,
};

int main() {
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/transparency\n");

    clearScreen();
    
    for (;;) {
        // Fill screen with 4 strips of different shade
        for (int i = 0; i<4; i++) {
            fillRect(SCR_W/4 * i, 0, SCR_W/4, SCR_H, bgColors[i], bgColors[i], bgColors[i]);
        }

        // For every semi-transparency (blending) mode
        for (int mode = 0; mode <= 3; mode++) {
            setSemiTransparencyMode(mode);

            // Draw semi transparent rectangles with different colors
            for (int x = 0; x < 8 * 4; x++) {
                const int S = 8;

                int r = 128 * ((x>>0)&1);
                int g = 128 * ((x>>1)&1);
                int b = 128 * ((x>>2)&1);
                int y = mode;

                TILE t;
                setTile(&t);
                setSemiTrans(&t, 1);
                setRGB0(&t, r, g, b);
                setXY0(&t, 1 + (S+2) * x, 64 + (S+16) * y);
                setWH(&t, S, S);

                DrawPrim(&t);
            }
        }

        VSync(0);
    }
    return 0;
}
