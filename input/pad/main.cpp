#include <stdio.h>
#include <stdlib.h>
#include <psxapi.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <psxpad.h>
#include <io.h>

#define SCR_W 320
#define SCR_H 240

static DISPENV disp;
static DRAWENV draw;

static char padBuffer[2][34];
static unsigned short buttons = 0xffff, prevButtons = 0xffff;

static bool BUTTON(uint16_t button)
{
    return (buttons & button) == 0 && ((prevButtons & button) != 0);
}

static void ButtonUpdate(void)
{
    prevButtons = buttons;
    buttons = ((PADTYPE*)padBuffer[0])->btn;
}

int main()
{
    printf("PAD test (PAD1)\n");

    ResetGraph(0);
    SetDefDispEnv(&disp, 0, 0, SCR_W, SCR_H);
    SetDefDrawEnv(&draw, 0, 0, SCR_W, SCR_H);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
    SetDispMask(1);

    InitPAD(padBuffer[0], 34, padBuffer[1], 34);
    StartPAD();
    ChangeClearPAD(0);

    while (1)
    {
        if (BUTTON(PAD_UP)) {
            printf("PAD_UP\n");
        }
        if (BUTTON(PAD_DOWN)) {
            printf("PAD_DOWN\n");
        }
        if (BUTTON(PAD_LEFT)) {
            printf("PAD_LEFT\n");
        }
        if (BUTTON(PAD_RIGHT)) {
            printf("PAD_RIGHT\n");
        }
        if (BUTTON(PAD_SQUARE)) {
            printf("PAD_SQUARE\n");
        }
        if (BUTTON(PAD_TRIANGLE)) {
            printf("PAD_TRIANGLE\n");
        }
        if (BUTTON(PAD_CROSS)) {
            printf("PAD_X\n");
        }
        if (BUTTON(PAD_CIRCLE)) {
            printf("PAD_CIRCLE\n");
        }
        if (BUTTON(PAD_SELECT)) {
            printf("PAD_SEL\n");
        }
        if (BUTTON(PAD_START)) {
            printf("PAD_START\n");
        }
        if (BUTTON(PAD_L3)) {
            printf("PAD_L3\n");
        }
        if (BUTTON(PAD_R3)) {
            printf("PAD_R3\n");
        }
        if (BUTTON(PAD_L2)) {
            printf("PAD_L2\n");
        }
        if (BUTTON(PAD_R2)) {
            printf("PAD_R2\n");
        }
        if (BUTTON(PAD_L1)) {
            printf("PAD_L1\n");
        }
        if (BUTTON(PAD_R1)) {
            printf("PAD_R1\n");
        }
        VSync(0);
        ButtonUpdate();
    }
    return 0;
}