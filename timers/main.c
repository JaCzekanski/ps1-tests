#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <psxsio.h>
#include "io.h"

DISPENV disp;
DRAWENV draw;

#define OT_LEN 8
unsigned int ot[2][OT_LEN];
int db = 0;

const int testCount = 11; // First result is omitted


void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, w, h);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
}

void setDisplayResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    PutDispEnv(&disp);
}

void initVideo()
{
    ResetGraph(0);
    setResolution(320, 240);
    SetDispMask(1);
}

void display()
{
    DrawSync(0);
    VSync(0);

    PutDrawEnv(&draw);
    DrawOTag(ot[db] + (OT_LEN - 1));

    db ^= 1;
    ClearOTagR(ot[db], OT_LEN);
    SetDispMask(1);
}

const char *modes[3][4] = {
    {"System clock",
     "Dot clock",
     "System clock",
     "Dot clock"},
    {"System clock",
     "Hblank",
     "System clock",
     "Hblank"},
    {"System clock",
     "System clock",
     "System clock/8",
     "System clock/8"},
};

const char *syncTypes[3][4] = {
        {"0 = Pause counter during Hblanks",
         "1 = Reset counter to 0 at Hblanks",
         "2 = Reset counter to 0 at Hblanks and pause outside of Hblank",
         "3 = Pause until Hblank occurs once, then switch to Free Run"},
        {"0 = Pause counter during Vblanks",
         "1 = Reset counter to 0 at Vblanks",
         "2 = Reset counter to 0 at Vblanks and pause outside of Vblank",
         "3 = Pause until Vblank occurs once, then switch to Free Run"},
        {"0 = Stop counter at current value",
         "1 = Free Run",
         "2 = Free Run",
         "3 = Stop counter at current value"},
};

const int resolutions[5] = {
    256,
    320,
    368,
    512,
    640
};

const char *dotclocksFrequencies[5] = {
    "5.32224 MHz",
    "6.65280 MHz",
    "7.60320 MHz",
    "10.64448 MHz",
    "13.30560 MHz",
};

const char* getDotclockFrequencyForCurrentResolution() {
    switch (disp.disp.w) {
        case 256: return dotclocksFrequencies[0];
        case 320: return dotclocksFrequencies[1];
        case 368: return dotclocksFrequencies[2];
        case 512: return dotclocksFrequencies[3];
        case 640: return dotclocksFrequencies[4];
    }
    return "";
}

void __attribute__((optimize("O1"))) delay(uint32_t cycles)
{
    uint32_t delay = cycles >> 2;
    do {
        __asm__ __volatile__ ("nop");
        --delay;
    } while (delay != 0);
}

volatile int currVsync = 0;
volatile int prevVsync = 0;

void vblank_irq() {
    currVsync = !currVsync;
    // printf("VBlank!\n");
}

void waitVsync() {
    while (currVsync == prevVsync) ;
    prevVsync = currVsync;
}

uint16_t initTimer(int timer, int mode) {
    uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevMode = read16(timerBase+4);
    write16(timerBase + 4, mode << 8);

    return prevMode;
}

uint16_t initTimerWithSync(int timer, int mode, int sync) {
    uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevMode = read16(timerBase+4);
    write16(timerBase + 4, (mode << 8) | ((sync&3) << 1) | 1);

    return prevMode;
}

void restoreTimer(int timer, uint16_t prevMode) {
    uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    write16(timerBase+4, prevMode);
}

void testTimerWithCyclesDelay(int timer, int mode, int cycles) {
    uint32_t results[testCount];
    const uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevState = initTimer(timer, mode);

    int32_t start = read16(timerBase);
    int32_t end;
    int32_t diff;
    uint16_t flags = 0;
    for (int i = 0; i < testCount; i++) {
        start = read16(timerBase);

        delay(cycles);

        end = read16(timerBase);
        flags = read16(timerBase + 4);
        if (flags & (1 << 12)) end += 0xffff;

        diff = end - start;

        results[i] = diff;
    }
    restoreTimer(timer, prevState);

    // Print results
    printf("Timer%d, clock: %14s, %4d cycles delay: ", timer, modes[timer][mode], cycles);
    for (int i = 1; i < testCount; i++) {
        printf("%6d, ", results[i]);
    }
    printf("\n");
}

void testTimerWithFrameDelay(int timer, int mode) {
    uint32_t results[testCount];
    const uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevState = initTimer(timer, mode);

    // Save previous VBlank callback - we will do vsync manually
    void* prevVblankCallback = GetInterruptCallback(0);
    InterruptCallback(0, vblank_irq);

    for (int i = 0; i < testCount; i++)
    {
        // Reset timer
        write16(timerBase + 4, mode << 8);

        waitVsync();

        int32_t value = read16(timerBase);
        uint16_t flags = read16(timerBase + 4);
        if (flags & (1 << 12)) value += 0xffff;

        results[i] = value;
    }
    restoreTimer(timer, prevState);

    InterruptCallback(0, vblank_irq);

    // Print results
    printf("Timer%d, clock: %14s,       frame delay: ", timer, modes[timer][mode]);
    for (int i = 1; i < testCount; i++) {
        printf("%6d, ", results[i]);
    }
    printf("\n");
}

void testTimerWithCyclesDelaySyncEnabled(int timer, int mode, int sync, int cycles) {
    uint32_t results[testCount];
    const uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevState = initTimerWithSync(timer, mode, sync);

    int32_t end;
    uint16_t flags = 0;
    for (int i = 0; i < testCount; i++) {
        delay(cycles);

        end = read16(timerBase);
        flags = read16(timerBase + 4);
        if (flags & (1 << 12)) end += 0xffff;

        results[i] = end;
    }
    restoreTimer(timer, prevState);

    // Print results
    printf("%-61s", syncTypes[timer][sync]);
    for (int i = 1; i < testCount; i++) {
        printf("%6d, ", results[i]);
    }
    printf("\n");
}

void runTestsForMode(int videoMode) {
    SetVideoMode(videoMode);
    printf("\nForced %s system.\n", (GetVideoMode() == MODE_NTSC) ? "NTSC" : "PAL");

    for (int timer = 0; timer < 3; timer++)
    {
        for (int mode = 0; mode < 2; mode++)
        {
            testTimerWithCyclesDelay(timer, (timer == 2) ? mode * 2 : mode, 1000);
            testTimerWithCyclesDelay(timer, (timer == 2) ? mode * 2 : mode, 5000);
            testTimerWithFrameDelay(timer, (timer == 2) ? mode * 2 : mode);
        }
    }

    // Only timer0 is dotclock dependent
    {
        const int timer = 0;
        for (int mode = 0; mode < 2; mode++) {
            for (int res = 0; res < 5; res++) {
                setDisplayResolution(resolutions[res], 240);

                const int delayCycles = 100;

                const int resW = disp.disp.w;
                const int resH = disp.disp.h;
                const char* freq = getDotclockFrequencyForCurrentResolution();
                printf("\nTesting Timer%d sync modes (clock source = %s, %d delay cycles, resolution = %dx%d, dotclock @ %s, not resetted between runs)\n", timer, modes[timer][mode], delayCycles, resW, resH, freq);
                for (int sync = 0; sync < 4; sync++) {
                    testTimerWithCyclesDelaySyncEnabled(timer, (timer == 2) ? mode * 2 : mode, sync, delayCycles);
                }
            }
        }
    }
    setDisplayResolution(320, 240);

    for (int timer = 1; timer <= 2; timer++) {
        for (int mode = 0; mode < 2; mode++) {
            const int delayCycles = 100;

            printf("\nTesting Timer%d sync modes (clock source = %s, %d delay cycles, not resetted between runs)\n", timer, modes[timer][mode], delayCycles);
            for (int sync = 0; sync < 4; sync++) {
                testTimerWithCyclesDelaySyncEnabled(timer, mode, sync, delayCycles);
            }
        }
    }
}

int main()
{
    printf("timers test\n");
    // printf("initializing Serial\n");
    // AddSIO(115200);
    initVideo();
    // FntLoad(SCREEN_XRES, 0);
 
    printf("Detected %s system.\n", (GetVideoMode() == MODE_NTSC) ? "NTSC" : "PAL");

    runTestsForMode(MODE_NTSC);
    // runTestsForMode(MODE_PAL);

    printf("\n\nDone, crashing now...\n");
    __asm__ volatile (".word 0xFC000000"); // Invalid opcode (63)
    return 0;
}