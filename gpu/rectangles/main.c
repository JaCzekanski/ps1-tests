#include <common.h>
#include "lena.h"

#define SCR_W 320
#define SCR_H 240

void setE1(int texPageX, int texPageY, int transparencyMode, int dithering) {
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* 15bit */ 2, transparencyMode, texPageX, texPageY);
    setDrawTPage(&e, /* Drawing to display area */ 1, dithering, texpage);
    DrawPrim(&e);
}

uint32_t randomColor(int i) {
    uint8_t r = 32 + (i * 1831) % 192;
    uint8_t g = 32 + (i * 2923) % 192;
    uint8_t b = 32 + (i * 5637) % 192;

    return (r << 0) | (g << 8) | (b << 16);
}

// drawRectangles will iterate through every Rectangle GPU command
// and send it to GPU with random but valid arguments
void drawRectangles(int _x, int _y) {
    for (int y = 0; y < 12; y++) {
        for (int x = 0; x < 16; x++) {
            uint8_t type = 0x60 + y * 16 + x;
            if (type > 0x7f) return;

            uint32_t color = randomColor(type); 
            
            uint32_t buf[5];
            buf[1] = (type << 24) | color;
            buf[2] = ((_y + y*20) << 16) | (_x + x*20);

            int cmdLength = 3;
            bool variableSize = ((type & 0b11000) >> 3) == 0;
            bool textured = (type & (1<<2)) != 0;

            if (textured) {
                uint16_t clut = 0;
                uint8_t u = rand()%64;
                uint8_t v = rand()%64;
                buf[cmdLength++] = (clut << 16) | (v << 8) | u; 
            }
            if (variableSize) {
                uint16_t w = 10 + rand()%10;
                uint16_t h = 10 + rand()%10;
                buf[cmdLength++] = (h << 16) | w;
            }

            setcode(buf, type);
            setlen(buf, cmdLength - 1);
            
            DrawPrim(buf);
        }
    }
}

int main() {
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/rectangles\n");

    clearScreenColor(0xff, 0xff, 0xff);

    // Load texture    
    TIM_IMAGE tim;
    GetTimInfo((unsigned int*)lena_tim, &tim);
    LoadImage(tim.prect, tim.paddr);

    for (;;) {
        srand(321);
        // TODO: Separate test for V/H flipping
        // Dithering should be ignored by rectangle commands
        setE1(tim.prect->x, tim.prect->y, 0, 0);
        drawRectangles(0, 0);

        setE1(tim.prect->x, tim.prect->y, 1, 1);
        drawRectangles(0, 64);
        
        setE1(tim.prect->x, tim.prect->y, 2, 1);
        drawRectangles(0, 128);
        
        setE1(tim.prect->x, tim.prect->y, 3, 1);
        drawRectangles(0, 192);

        VSync(0);
    }
    return 0;
}
