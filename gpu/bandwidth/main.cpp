#include <stdio.h>
#include <stdlib.h>
#include <psxapi.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <io.h>
#include <gpu.h>
#include <timer.h>

#define SCR_W 320
#define SCR_H 240

bool semitransparent = false;

uint32_t getTimer() {
    uint32_t value = readTimer(1);
    if (timerDidOverflow(1)) value += 0xffff;
    return value;
}

void fillScreen() {
    FILL f;
    
    setFill(&f);
    setRGB0(&f, rand()%255, rand()%255, rand()%255);
    setXY0(&f, 0, 0);
    setWH(&f, SCR_W, SCR_H);

    DrawPrim(&f);
}

void rectScreen() {
    TILE t;
    setTile(&t);
    setSemiTrans(&t, semitransparent);
    setXY0(&t, 0, 0);
    setWH(&t, SCR_W, SCR_H);
    setRGB0(&t, rand()%255, rand()%255, rand()%255);
    
    DrawPrim(&t);
}

void rectTexturedScreen() {
    SPRT s;
    setSprt(&s);
    setSemiTrans(&s, semitransparent);
    setXY0(&s, 0, 0);
    setWH(&s, SCR_W, SCR_H);
    setRGB0(&s, rand()%255, rand()%255, rand()%255);
    setClut(&s, 768, 256);
    setUV0(&s,
        0, 0
    );
   
    DrawPrim(&s);
}

int xOffset = 0;

void quadScreen() {
    POLY_F4 p;
    setPolyF4(&p);
    setSemiTrans(&p, semitransparent);
    setXY4(&p, 
        xOffset + 0, 0,
        xOffset + SCR_W, 0,
        xOffset + 0, SCR_H,
        xOffset + SCR_W, SCR_H
    );
    setRGB0(&p, rand()%255, rand()%255, rand()%255);
    
    DrawPrim(&p);
}

#define setUVWHfixed( p, _u0, _v0, _w, _h ) 		\
	(p)->u0 = _u0, 		(p)->v0 = _v0,		\
	(p)->u1 = _u0+(_w),	(p)->v1 = _v0,		\
	(p)->u2 = _u0, 		(p)->v2 = _v0+(_h),	\
	(p)->u3 = _u0+(_w), (p)->v3 = _v0+(_h)

void quadTexturedScreen() {
    POLY_FT4 p;
    setPolyFT4(&p);
    setSemiTrans(&p, semitransparent);
    setShadeTex(&p, 1);
    setXY4(&p, 
        0, 0,
        SCR_W, 0,
        0, SCR_H,
        SCR_W, SCR_H
    );
    setTPage(&p, 2, 0, 512, 256);
    setClut(&p, 768, 256);
    setUVWHfixed(&p,
        0, 0,
        255, 255
    );
    setRGB0(&p, 0x80, 0x80, 0x80);//rand()%255, rand()%255, rand()%255);
    
    DrawPrim(&p);
}

constexpr int callCount = 400;

void calculate(const char* test, uint16_t hblanks) {
    const int scanlinesPerFrame = 263;
    const int framesPerSecond = 60;
    uint32_t diff = hblanks;
    uint32_t dt_q = (diff*1000) / (framesPerSecond*scanlinesPerFrame);
    uint32_t dt_r = (diff*1000) % (framesPerSecond*scanlinesPerFrame);

    uint32_t bytesPerFrame = SCR_W * SCR_H * 2;
    uint32_t bytesSum = bytesPerFrame * callCount;

    uint32_t megaBytesPerSecond;
    if (dt_q == 0) { // workaround for missing div 0 handler
        megaBytesPerSecond = 99999999;
    } else {
        megaBytesPerSecond = bytesSum / (dt_q*1024);
    }
    printf("%-30s dT: %5d.%-5d ms (hblanks: %5d), speed: %d MB/s\n", test, dt_q, dt_r, diff, megaBytesPerSecond);
}

uint16_t* buffer = (uint16_t*)0x801a0000;

int main() {
    initVideo(SCR_W, SCR_H);
    SetVideoMode(MODE_NTSC);
    printf("\ngpu/bandwidth\n");

    uint16_t oldTimer1Mode = initTimer(1, 1); // Timer1, HBlank

    auto test = [](const char* name, void (*func)(), bool semi = false) {
        clearScreen();

        DrawSync(0);
        resetTimer(1);
        semitransparent = semi;
        for (int i = 0; i < callCount; i++) {
            func();
        }
        calculate(name, getTimer());
    };

    EnterCriticalSection();
    test("vramToCpu", []() { vramReadDMA(0, 0, SCR_W, SCR_H, buffer); });
    test("cpuToVram", []() { vramWriteDMA(0, 0, SCR_W, SCR_H, buffer); });
    test("vramToVram", []() { vramToVramCopy(0, 0, 320, 0, SCR_W, SCR_H); });
    test("FillScreen GP0(2)", fillScreen);
    test("Rectangle", rectScreen, false);
    test("Rectangle (semitransparent)", rectScreen, true);
    test("Rectangle textured", rectTexturedScreen, false);
    test("Rectangle textured (semi)", rectTexturedScreen, true);
    test("Polygon quad", quadScreen, false);
    test("Polygon quad (semi)", quadScreen, true);
    test("Polygon quad textured", quadTexturedScreen, false);
    test("Polygon quad textured (semi)", quadTexturedScreen, true);

    xOffset = -SCR_W/4;
    test("Polygon quad (1/4 off screen)", quadScreen, false);

    xOffset = -SCR_W/2;
    test("Polygon quad (1/2 off screen)", quadScreen, false);
    ExitCriticalSection();

    restoreTimer(1, oldTimer1Mode);

    printf("Done\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}
