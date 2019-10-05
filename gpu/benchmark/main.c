#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxetc.h>
#include "io.h"

#define OT_LEN 		8

unsigned int ot[2][OT_LEN];		/* Ordering tables */
int db = 0;						/* Double buffer index */

char pribuff[2][4096];			/* Primitive packet buffers */
char* ptr;

DISPENV disp;
DRAWENV draw;

#define SCR_W 320
#define SCR_H 240

void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, w, h);

    draw.dtd=0; // Disable dither

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
    FILL* f = (FILL*)ptr;
    ptr += sizeof(FILL);
    
    setFill(f);
    setRGB0(f, 0xff, 0xff, 0xff);
    setXY0(f, 0, 0);
    setWH(f, 1023, 511);

    addPrim(ot[db] + (OT_LEN-1), f);
}

int main()
{
    initVideo();
    printf("\ngpu/benchmark\n");

    PutDrawEnv(&draw);

    for (;;) {
        srand(1234);
        ptr = pribuff[db];

		ClearOTagR( ot[db] + (OT_LEN-1), OT_LEN );
        for (int i = 0; i < 100; i++) {
            POLY_G4 *p = (POLY_G4*)ptr;
            ptr += sizeof(POLY_G4);
            setPolyG4(p);
            setSemiTrans(p, 0); // Disable transparency

            int x[4], y[4];
            x[0] = rand()%SCR_W;  y[0] = rand()%SCR_H;
            x[1] = rand()%SCR_W;  y[1] = rand()%SCR_H;
            x[2] = rand()%SCR_W;  y[2] = rand()%SCR_H;
            x[3] = rand()%SCR_W;  y[3] = rand()%SCR_H;

            // Center
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            setRGB2(p, rand()%255, rand()%255, rand()%255);
            setRGB3(p, rand()%255, rand()%255, rand()%255);
            setXY4(p,
                x[0], y[0],
                x[1], y[1],
                x[2], y[2],
                x[3], y[3]
            );
            addPrim(ot[db] + (OT_LEN-1), p);
        }
        clearScreen(); // ? reverse ordering
        

        DrawOTag(ot[db] + (OT_LEN-1));
        DrawSync(0);
        db = !db;
        
        VSync(0);
    }


    printf("Done, crashing now...\n");
    __asm__ volatile (".word 0xFC000000"); // Invalid opcode (63)
    return 0;
}
