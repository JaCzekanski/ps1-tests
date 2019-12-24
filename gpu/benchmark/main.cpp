#include <common.h>
#include <io.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include "lena.h"
#include "type_traits.hpp"

#define SCR_W 320
#define SCR_H 240

#define OT_LEN 2
#define PACKET_LEN 65535

struct DB {
    DISPENV disp;
    DRAWENV draw;
    uint32_t ot[OT_LEN];
    uint32_t ptr[PACKET_LEN];
};

DB db[2]; 
int activeDb = 0; // Double buffering index
uint32_t* ptr;

char padBuffer[2][34];
unsigned short buttons = 0xffff, prevButtons = 0xffff;

TIM_IMAGE tim;

bool dithering = false;
bool semiTransparency = false;

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

        ClearOTagR(db[i].ot, OT_LEN);
    }
    activeDb = 0;

	PutDrawEnv(&db[activeDb].draw);
    ptr = db[activeDb].ptr;

    SetDispMask(1);

	InitPAD(padBuffer[0], 34, padBuffer[1], 34);
	StartPAD();
	ChangeClearPAD(0);

	FntLoad(960, 0);
	FntOpen(0, 16, SCR_W, SCR_H-32, 0, 100);	
}

void advanceRand(int n) {
    for (int i = 0; i<n; i++) rand();
}

#define ASSERT(x, msg) do { \
    if (!(x)) { \
        printf("ASSERTION FAILED: "#msg"\n"); \
        __asm__ volatile (".word 0xFC000000"); \
    } \
} while (0)

template<typename T, bool shaded = false>
void drawPolygons(int n) {
    for (int i = 0; i < n; i++) {
        T *p = (T*)ptr;
        ptr += sizeof(T);

        constexpr bool isTriangle = IS_SAME(T, POLY_F3)  || IS_SAME(T, POLY_G3) || IS_SAME(T, POLY_FT3) || IS_SAME(T, POLY_GT3);
        constexpr bool isQuad     = IS_SAME(T, POLY_F4)  || IS_SAME(T, POLY_G4) || IS_SAME(T, POLY_FT4) || IS_SAME(T, POLY_GT4);
        constexpr bool has3Colors = IS_SAME(T, POLY_G3)  || IS_SAME(T, POLY_GT3);
        constexpr bool has4Colors = IS_SAME(T, POLY_G4)  || IS_SAME(T, POLY_GT4);
        constexpr bool isTextured = IS_SAME(T, POLY_FT3) || IS_SAME(T, POLY_GT3) || IS_SAME(T, POLY_FT4) || IS_SAME(T, POLY_GT4);

        if constexpr (IS_SAME(T, POLY_F3))  setPolyF3(p);
        else if      (IS_SAME(T, POLY_G3))  setPolyG3(p);
        else if      (IS_SAME(T, POLY_FT3)) setPolyFT3(p);
        else if      (IS_SAME(T, POLY_GT3)) setPolyGT3(p);
        else if      (IS_SAME(T, POLY_F4))  setPolyF4(p);
        else if      (IS_SAME(T, POLY_G4))  setPolyG4(p);
        else if      (IS_SAME(T, POLY_FT4)) setPolyFT4(p);
        else if      (IS_SAME(T, POLY_GT4)) setPolyGT4(p);
        else    ASSERT(false, "Invalid type");

        setSemiTrans(p, semiTransparency);

        p->x0 = rand() % SCR_W;  p->y0 = rand() % SCR_H;
        p->x1 = rand() % SCR_W;  p->y1 = rand() % SCR_H;
        p->x2 = rand() % SCR_W;  p->y2 = rand() % SCR_H;
        if constexpr (isQuad) {
            p->x3 = rand() % SCR_W;  p->y3 = rand() % SCR_H;
        } else {
            advanceRand(2);
        }

        if constexpr (!has3Colors && !has4Colors) {
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            advanceRand(9);
        } else if constexpr (has3Colors) {
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            setRGB2(p, rand()%255, rand()%255, rand()%255);
            advanceRand(3);
        } else if constexpr (has4Colors) {
            setRGB0(p, rand()%255, rand()%255, rand()%255);
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            setRGB2(p, rand()%255, rand()%255, rand()%255);
            setRGB3(p, rand()%255, rand()%255, rand()%255);
        }

        if constexpr (isTextured) {
            setShadeTex(p, !shaded);
            setTPage(p, 2, 0, tim.prect->x, tim.prect->y);
            setClut(p, tim.crect->x, tim.crect->y);
            
            p->u0 = rand() % tim.prect->w;  p->u0 = rand() % tim.prect->h;
            p->u1 = rand() % tim.prect->w;  p->u1 = rand() % tim.prect->h;
            p->u2 = rand() % tim.prect->w;  p->u2 = rand() % tim.prect->h;
            if constexpr (isQuad) {
                p->u3 = rand() % tim.prect->w;  p->v3 = rand() % tim.prect->h;
            } else {
                advanceRand(2);
            }
        } else {
            advanceRand(8);
        }

        addPrim(&db[activeDb].ot[OT_LEN-1], p);
    }
}

template<typename T, bool shaded = false>
void drawRects(int n) {
    for (int i = 0; i < n; i++) {
        T *p = (T*)ptr;
        ptr += sizeof(T);

        constexpr bool isTextured = IS_SAME(T, SPRT);

        if constexpr (IS_SAME(T, TILE))  setTile(p);
        else if      (IS_SAME(T, SPRT))  setSprt(p);
        else    ASSERT(false, "Invalid type");

        setSemiTrans(p, semiTransparency);

        p->x0 = rand() % SCR_W;  p->y0 = rand() % SCR_H;
        p->w  = rand() % SCR_W;  p->h  = rand() % SCR_H;
        advanceRand(4);

        setRGB0(p, rand()%255, rand()%255, rand()%255);
        advanceRand(9);

        if constexpr (isTextured) {
            setShadeTex(p, !shaded);
            setClut(p, tim.crect->x, tim.crect->y);
    
            p->u0 = rand() % tim.prect->w;  p->u0 = rand() % tim.prect->h;
            advanceRand(6);
        } else {
            advanceRand(8);
        }

        addPrim(&db[activeDb].ot[OT_LEN-1], p);
    }
    
    // Reverse order
    DR_TPAGE *e = (DR_TPAGE*)ptr;
    ptr += sizeof(DR_TPAGE);
    setDrawTPage(e, 0, dithering, getTPage(2, 0, tim.prect->x, tim.prect->y));
    addPrim(&db[activeDb].ot[OT_LEN-1], e);
}

template<typename T>
void drawLines(int n) {
    for (int i = 0; i < n; i++) {
        T *p = (T*)ptr;
        ptr += sizeof(T);

        if constexpr (IS_SAME(T, LINE_F2))  setLineF2(p);
        else if      (IS_SAME(T, LINE_G2))  setLineG2(p);
        else    ASSERT(false, "Invalid type");

        setSemiTrans(p, semiTransparency);

        p->x0 = rand() % SCR_W;  p->y0 = rand() % SCR_H;
        p->x1 = rand() % SCR_W;  p->y1 = rand() % SCR_H;
        advanceRand(4);

        setRGB0(p, rand()%255, rand()%255, rand()%255);
        if constexpr (IS_SAME(T, LINE_G2)) {
            setRGB1(p, rand()%255, rand()%255, rand()%255);
            advanceRand(6);
        } else {
            advanceRand(9);
        }

        advanceRand(8);

        addPrim(&db[activeDb].ot[OT_LEN-1], p);
    }
}

uint32_t diff = 0;
uint32_t frametime = 0;
uint32_t fps = 0;

void swapBuffers() {
    FntFlush(-1);

    DrawSync(0); // Wait for GPU to finish rendering 
    
    const uint32_t timerBase = 0x1f801100 + (1 * 0x10);
    static uint32_t prevTimer = 0;
    uint32_t timer = read16(timerBase);

    diff = timer - prevTimer;
    frametime = (diff * 1000) / 263;
    fps = 1000 / frametime;
    
    prevTimer = timer;

    VSync(0);

    activeDb = !activeDb;
    ptr = db[activeDb].ptr;

    ClearOTagR(db[activeDb].ot, OT_LEN);

    PutDrawEnv(&db[activeDb].draw);
    PutDispEnv(&db[activeDb].disp);

    DrawOTag(&db[1 - activeDb].ot[OT_LEN - 1]);

	prevButtons = buttons;
	buttons = ((PADTYPE*)padBuffer[0])->btn;

}

constexpr const char* modeStr[] = {
    "Triangle flat",
    "Triangle gouraud",
    "Triangle textured",
    "Triangle textured flat",
    "Triangle textured gouraud",
    "Quad flat",
    "Quad gouraud",
    "Quad textured",
    "Quad textured flat",
    "Quad textured gouraud",
    "Rect",
    "Rect textured",
    "Rect textured flat",
    "Line flat",
    "Line gouraud",
};
constexpr int modeCount = sizeof(modeStr) / sizeof(const char*);

int main() {
    init();
    printf("\ngpu/benchmark\n");

    // Load texture    
    GetTimInfo((unsigned int*)lena_tim, &tim);
	LoadImage(tim.prect, tim.paddr);
	if(tim.mode & 0x8) {
		LoadImage(tim.crect, tim.caddr);
	}
	DrawSync(0);

    int mode = 0;
    int n = 100;

    for (;;) {
		if (BUTTON(PAD_UP)) {
            if (mode > 0) mode--;
            printf("Mode: %s\n", modeStr[mode]);
        }
		if (BUTTON(PAD_DOWN)) {
            if (mode < modeCount - 1) mode++;
            printf("Mode: %s\n", modeStr[mode]);
        }
        int step = 10;
        if ((buttons & (PAD_L1)) == 0) step = 1;
        if ((buttons & (PAD_L2)) == 0) step = 50;
		if (BUTTON(PAD_LEFT)) {
            n -= step;
            if (n < 0) n = 0;
            printf("Poly count: %d\n", n);
        }
		if (BUTTON(PAD_RIGHT)) {
            n += step;
            if (n > 1000) n = 1000;
            printf("Poly count: %d\n", n);
        }
		if (BUTTON(PAD_CROSS)) {
            semiTransparency = !semiTransparency;
            printf("SemiTransparency: %d\n", semiTransparency);
        }
		if (BUTTON(PAD_CIRCLE)) {
            dithering = !dithering;
            for (int i = 0; i<2; i++) {
                db[i].draw.dtd = dithering;
            }
            printf("Dithering: %d\n", dithering);
        }
        srand(3487);

        if      (mode == 0)  drawPolygons<POLY_F3       >(n);
        else if (mode == 1)  drawPolygons<POLY_G3       >(n);
        else if (mode == 2)  drawPolygons<POLY_FT3      >(n);
        else if (mode == 3)  drawPolygons<POLY_FT3, true>(n);
        else if (mode == 4)  drawPolygons<POLY_GT3, true>(n);
        else if (mode == 5)  drawPolygons<POLY_F4       >(n);
        else if (mode == 6)  drawPolygons<POLY_G4       >(n);
        else if (mode == 7)  drawPolygons<POLY_FT4      >(n);
        else if (mode == 8)  drawPolygons<POLY_FT4, true>(n);
        else if (mode == 9)  drawPolygons<POLY_GT4, true>(n);
        else if (mode == 10) drawRects   <TILE          >(n);
        else if (mode == 11) drawRects   <SPRT          >(n);
        else if (mode == 12) drawRects   <SPRT,     true>(n);
        else if (mode == 13) drawLines   <LINE_F2       >(n);
        else if (mode == 14) drawLines   <LINE_G2       >(n);
        // TODO: Polylines

		FntPrint(-1, "(^ v) Mode: %s\n", modeStr[mode]);
		FntPrint(-1, "(< >) Poly count: %d\n", n);
		FntPrint(-1, "( X ) Transparency: %d\n", semiTransparency);
		FntPrint(-1, "(  O) Dithering: %d\n", dithering);

        swapBuffers();
    }

    return 0;
}
