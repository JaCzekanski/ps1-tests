#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <io.h>
#include <gpu.h>
#include <timer.h>

typedef char bool;

#define SCR_W 320
#define SCR_H 240

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

void rectScreen(bool semitransparent) {
    TILE t;
    setTile(&t);
    setSemiTrans(&t, semitransparent);
    setXY0(&t, 0, 0);
    setWH(&t, SCR_W, SCR_H);
    setRGB0(&t, rand()%255, rand()%255, rand()%255);
    
    DrawPrim(&t);
}

void rectTexturedScreen(bool semitransparent) {
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

void quadScreen(bool semitransparent) {
    POLY_F4 p;
    setPolyF4(&p);
    setSemiTrans(&p, semitransparent);
    setXY4(&p, 
        0, 0,
        SCR_W, 0,
        0, SCR_H,
        SCR_W, SCR_H
    );
    setRGB0(&p, rand()%255, rand()%255, rand()%255);
    
    DrawPrim(&p);
}

#define setUVWHfixed( p, _u0, _v0, _w, _h ) 		\
	(p)->u0 = _u0, 		(p)->v0 = _v0,		\
	(p)->u1 = _u0+(_w),	(p)->v1 = _v0,		\
	(p)->u2 = _u0, 		(p)->v2 = _v0+(_h),	\
	(p)->u3 = _u0+(_w), (p)->v3 = _v0+(_h)

void quadTexturedScreen(bool semitransparent) {
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

const int callCount = 400;

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

const char* testCases[9] = {
    "FillScreen GP0(2)",
    "Rectangle",
    "Rectangle (semitransparent)",
    "Rectangle textured",
    "Rectangle textured (semi)",
    "Polygon quad",
    "Polygon quad (semi)",
    "Polygon quad textured",
    "Polygon quad textured (semi)"
};

uint16_t* buffer = (uint16_t*)0x801a0000;

void testVramToCpu() {
    DrawSync(0);
    resetTimer(1);
    for (int i = 0; i < callCount; i++) {
        vramReadDMA(0, 0, SCR_W, SCR_H, buffer);
    }
    calculate("vramToCpu", getTimer());
}

void testCpuToVram() {
    DrawSync(0);
    resetTimer(1);
    for (int i = 0; i < callCount; i++) {
        vramWriteDMA(0, 0, SCR_W, SCR_H, buffer);
    }
    calculate("cpuToVram", getTimer());
}

void testVramToVram() {
    DrawSync(0);
    resetTimer(1);
    for (int i = 0; i < callCount; i++) {
        vramToVramCopy(0, 0, 320, 0, SCR_W, SCR_H);
    }
    calculate("vramToVram", getTimer());
}

int main() {
    initVideo(SCR_W, SCR_H);
    SetVideoMode(MODE_NTSC);
    printf("\ngpu/bandwidth\n");

    uint16_t oldTimer1Mode = initTimer(1, 1); // Timer1, HBlank

    testVramToCpu();
    testCpuToVram();
    testVramToVram();

    for (int test = 0; test<9; test++) {
        clearScreen();

        DrawSync(0);
        resetTimer(1);
        for (int i = 0; i < callCount; i++) {
            switch (test) {
                case 0: 
                    fillScreen();
                    break;
                case 1: 
                    rectScreen(false);
                    break;
                case 2: 
                    rectScreen(true);
                    break;
                case 3:
                    rectTexturedScreen(false);
                    break;
                case 4:
                    rectTexturedScreen(true);
                    break;
                case 5: 
                    quadScreen(false);
                    break;
                case 6: 
                    quadScreen(true);
                    break;
                case 7: 
                    quadTexturedScreen(false);
                    break;
                case 8: 
                    quadTexturedScreen(true);
                    break;
            }
        }
        calculate(testCases[test], getTimer());
    }

    restoreTimer(1, oldTimer1Mode);

    printf("Done\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}
