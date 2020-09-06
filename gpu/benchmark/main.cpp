#include <common.h>
#include <io.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <measure.hpp>
#include "lena.h"
#include "modes.h"

uint32_t drawListBuffer[PACKET_LEN]; 
uint32_t drawListOt;
uint32_t* ptr = drawListBuffer;

struct DB { 
    DISPENV disp;
    DRAWENV draw;
};

DB db[2]; 
int activeDb = 0; // Double buffering index
int frame = 0;

char padBuffer[2][34];
uint16_t buttons = 0, prevButtons = 0;

TIM_IMAGE texture;

bool dithering = false;
int semiTransparency = 0;

bool drawGui = true;
int selectedMode = 0;
int polyCount = 100;
uint32_t pixelsDrawn = 0;

bool BUTTON(uint16_t button) {
	return (buttons & button) == 0 && ((prevButtons & button) != 0);
}

void init() {
    ResetGraph(0);
    SetVideoMode(MODE_NTSC);
    for (int i = 0; i<=1; i++) {
        SetDefDispEnv(&db[i].disp, !i ? 0 : SCR_W, 0, SCR_W, SCR_H);
        SetDefDrawEnv(&db[i].draw, !i ? SCR_W : 0, 0, SCR_W, SCR_H);

        db[i].disp.isinter = false; // Progressive mode
        db[i].disp.isrgb24 = false; // 16bit mode

        db[i].draw.dtd = dithering; // Disable dither
        db[i].draw.isbg = true; // Clear bg on PutDrawEnv

        setRGB0(&db[i].draw, 0, 0, 0);
    }
    activeDb = 0;

	PutDrawEnv(&db[activeDb].draw);
    PutDispEnv(&db[activeDb].disp);

    SetDispMask(1);

	InitPAD(padBuffer[0], 34, padBuffer[1], 34);
	StartPAD();
	ChangeClearPAD(0);

	FntLoad(960, 0);
	FntOpen(0, 16, SCR_W, SCR_H-32, 0, 2048);	
}

void advanceRand(int n) {
    for (int i = 0; i<n; i++) rand();
}

void drawRectangle(int x, int y, int w, int h) {
    DR_TPAGE e;
    setDrawTPage(&e, 0, dithering, getTPage(0, 0, 0, 0));
    DrawPrim(&e);

    TILE r;
    setTile(&r);
    setSemiTrans(&r, true);
    setXY0(&r, x, y);
    setWH(&r, w, h);
    setRGB0(&r, 0, 0, 0);

    DrawPrim(&r);
}

void renderFrame() { 
    activeDb = !activeDb; 

    PutDrawEnv(&db[activeDb].draw);
    PutDispEnv(&db[activeDb].disp);

    DrawSync(0);
    auto hblanks = measure<MeasureMethod::HBlank>([]{
        DrawOTag(&drawListOt); // Render drawList
        DrawSync(0); // Wait for GPU to finish rendering         
    });
    
    if (drawGui) {
        FntPrint(-1, "(^ v) Mode: %s\n", modes[selectedMode].description);
        FntPrint(-1, "(< >) Poly count: %d (L1 x1, L2 x50)\n", polyCount);
        FntPrint(-1, "( X ) Transparency: %s\n", semiTransparencyModesStr[semiTransparency]);
        FntPrint(-1, "(  O) Dithering: %d\n\n", dithering);
        FntPrint(-1, "Select: GUI off\n");

        const int scanlinesPerFrame = 263;
        const int framesPerSecond = 60;
        uint32_t dt_q = (hblanks*1000) / (framesPerSecond*scanlinesPerFrame);
        uint32_t dt_r = (hblanks*1000) % (framesPerSecond*scanlinesPerFrame);
        
        FntPrint(-1, "dt: %d.%dms\n", dt_q, dt_r / 100);
        if (dt_q != 0) {
            FntPrint(-1, "VPS: %d\n", 10000000 / (dt_q*1000 + dt_r));
        }
        FntPrint(-1, "Pixels drawn: %lld\n", pixelsDrawn);

        // Render text frame
        drawRectangle(0, 0, 300, 80);

        // Render text
        FntFlush(-1);
    }

    VSync(0);
}

void generateDrawList() {
    srand(3487);

    ClearOTagR(&drawListOt, 1);
    ptr = drawListBuffer;
    modes[selectedMode].func(polyCount);
}

int main() {
    init();
    printf("\ngpu/benchmark\n");

    // Load texture    
    GetTimInfo((unsigned int*)lena_tim, &texture);
	LoadImage(texture.prect, texture.paddr);
	if(texture.mode & 0x8) {
		LoadImage(texture.crect, texture.caddr);
	}
	DrawSync(0);

    generateDrawList();
    for (frame = 0;;frame++) {
        prevButtons = buttons;
        buttons = ((PADTYPE*)padBuffer[0])->btn;

		if (BUTTON(PAD_UP)) {
            if (selectedMode > 0) {
                selectedMode--;
                printf("Mode: %s\n", modes[selectedMode].description);

                generateDrawList();
            }
        }
		if (BUTTON(PAD_DOWN)) {
            if (selectedMode < (sizeof(modes) / sizeof(RenderingMode)) - 1) {
                selectedMode++;
                printf("Mode: %s\n", modes[selectedMode].description);

                generateDrawList();
            }
        }
        int step = 10;
        if ((buttons & (PAD_L1)) == 0) step = 1;
        if ((buttons & (PAD_L2)) == 0) step = 50;
		if (BUTTON(PAD_LEFT)) {
            polyCount -= step;
            if (polyCount < 0) polyCount = 0;
            printf("Poly count: %d\n", polyCount);
    
            generateDrawList();
        }
		if (BUTTON(PAD_RIGHT)) {
            polyCount += step;
            if (polyCount > 1000) polyCount = 1000;
            printf("Poly count: %d\n", polyCount);
    
            generateDrawList();
        }
		if (BUTTON(PAD_CROSS)) {
            if (++semiTransparency > 4) semiTransparency = 0;
            for (int i = 0; i<2; i++) {
                db[i].draw.tpage = getTPage(/* bits */ 0, (semiTransparency > 0) ? semiTransparency - 1 : 0, /* texPageX */ 0, /* texPageY */ 0);
            }
            printf("SemiTransparency: %s\n", semiTransparencyModesStr[semiTransparency]);
       
            generateDrawList();
        }
		if (BUTTON(PAD_CIRCLE)) {
            dithering = !dithering;
            for (int i = 0; i<2; i++) {
                db[i].draw.dtd = dithering;
            }
            printf("Dithering: %d\n", dithering);
       
            generateDrawList();
        }
		if (BUTTON(PAD_SELECT)) {
            drawGui = !drawGui;
        }

        renderFrame();
    }

    return 0;
}
