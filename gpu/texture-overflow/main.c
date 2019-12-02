#include <common.h>
#include "lena.h"
#include "cube.h"

#define SCR_W 320
#define SCR_H 240

void setTexPage(int x, int y) {
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* 15bit */ 2, /* semi-transparency mode */ 0, /*x*/x, /*y*/y);
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ 0, texpage);
    DrawPrim(&e);
}

int main() {
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/texture-overflow\n");

    clearScreenColor(0xff, 0x00, 0x00);

    TIM_IMAGE tim;
    GetTimInfo((unsigned int*)lena_tim, &tim);
    LoadImage(tim.prect, tim.paddr);
    
    GetTimInfo((unsigned int*)cube_tim, &tim);
    LoadImage(tim.prect, tim.paddr);

    for (;;) {
        fillRect(0, 0, SCR_W, SCR_H, 0xff, 0xff, 0xff);

        setTexPage(896, 256);

        SPRT s;
        setSprt(&s);
        setSemiTrans(&s, 0);
        setRGB0(&s, 128, 128, 128 );
        s.x0 = 16;
        s.y0 = 16;
        s.w = 256;
        s.h = 128;
        s.u0 = 0;
        s.v0 = 0;

        DrawPrim(&s);

        VSync(0);
    }
    return 0;
}
