#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxetc.h>
#include <psxsio.h>
#include <psxapi.h>
#include <io.h>
#include "timer.h"

DISPENV disp;
DRAWENV draw;

#define OT_LEN 8
unsigned int ot[2][OT_LEN];
int db = 0;

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

void restoreTimer(int timer, uint16_t prevMode) {
    uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    write16(timerBase+4, prevMode);
}

const uint32_t SIO_CTRL = 0x1F80105A;
void sioInit() {
    write16(SIO_CTRL, 1<<6); // Reset
}

#define sioGetRts() (((*((uint16_t*)SIO_CTRL)) & (1 << 5)) == 0)
#define sioSetRts(state) (*((uint16_t*)SIO_CTRL) = (!state) << 5)

struct TimerState {
    bool gpio;
    uint16_t counter;
    uint16_t mode;
    uint16_t target;
    uint16_t istat;
};

union CounterMode {
    enum class SyncMode0 : uint16_t {
        pauseDuringHblank = 0,
        resetAtHblank = 1,
        resetAtHblankAndPauseOutside = 2,
        pauseUntilHblankAndFreerun = 3
    };
    enum class SyncMode1 : uint16_t {
        pauseDuringVblank = 0,
        resetAtVblank = 1,
        resetAtVblankAndPauseOutside = 2,
        pauseUntilVblankAndFreerun = 3
    };
    enum class SyncMode2 : uint16_t {  //
        stopCounter = 0,
        freeRun = 1,
        freeRun_ = 2,
        stopCounter_ = 3
    };

    enum class ResetToZero : uint16_t { whenFFFF = 0, whenTarget = 1 };
    enum class IrqRepeatMode : uint16_t { oneShot = 0, repeatedly = 1 };
    enum class IrqPulseMode : uint16_t { shortPulse = 0, toggle = 1 };
    enum class ClockSource0 : uint16_t { systemClock = 0, dotClock = 1 };
    enum class ClockSource1 : uint16_t { systemClock = 0, hblank = 1 };
    enum class ClockSource2 : uint16_t { systemClock = 0, systemClock_8 = 1 };

    struct {
        uint16_t syncEnabled : 1;
        uint16_t syncMode : 2;
        ResetToZero resetToZero : 1;

        uint16_t irqWhenTarget : 1;
        uint16_t irqWhenFFFF : 1;
        IrqRepeatMode irqRepeatMode : 1;
        IrqPulseMode irqPulseMode : 1;

        // For all timer different clock sources are available
        uint16_t clockSource : 2;
        uint16_t interruptRequest : 1;  // R
        uint16_t reachedTarget : 1;     // R
        uint16_t reachedFFFF : 1;       // R

        uint16_t : 3;
    };

    uint16_t _reg;
    uint8_t _byte[2];

    CounterMode(uint16_t r) : _reg(r) {}
};

void explain(CounterMode mode) {
    printf("\nTimer mode register explained:\n");
    printf("Sync enabled: %d\n", mode.syncEnabled);
    printf("Reset counter to 0x0000: %s\n", mode.resetToZero == CounterMode::ResetToZero::whenFFFF ? "when 0xffff" : "when target");
    printf("IRQ when target: %d\n", mode.irqWhenTarget);
    printf("IRQ when 0xffff: %d\n", mode.irqWhenFFFF);
    printf("IRQ repeat mode: %s\n", mode.irqRepeatMode == CounterMode::IrqRepeatMode::oneShot ? "one-shot" : "repeatedly");
    printf("IRQ pulse mode: %s\n", mode.irqPulseMode == CounterMode::IrqPulseMode::shortPulse ? "short pulse" : "toggle");
    printf("Clock source: %d\n\n", (int)mode.clockSource);
}

void __attribute__((optimize("Os"))) testDumpTimerValuesCycleByCycle(CounterMode mode) {
    const int timer = 0;
    const int target = 5;

    // Count from 0 to TARGET (included)
    printf("\nDump all timer registers after every clock cycle\n");
    explain(mode);

    const uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevState = read16(timerBase + 4);
    
    EnterCriticalSection();

    bool rts = false;
    sioSetRts(rts);

    write16(timerBase + 8, target);
    write16(timerBase + 4, mode._reg);
    write16(timerBase, 0); // Reset current value
    read16(timerBase + 4); // Reset reached bits

    #define TICKS 26
    struct TimerState states[TICKS];

    volatile const uint16_t* COUNTER = ((uint16_t*)timerBase);
    volatile const uint16_t* MODE = ((uint16_t*)(timerBase + 4));
    volatile const uint16_t* TARGET = ((uint16_t*)(timerBase + 8));
    volatile const uint16_t* ISTAT = (uint16_t*)(0x1F801070);
    volatile uint16_t* SIO_CTRL = (uint16_t*)0x1F80105A;

    for (int i = 0; i<TICKS; i++) {
        states[i].gpio = rts;
        states[i].counter = *COUNTER;
        states[i].mode = *MODE;
        states[i].target = *TARGET;
        states[i].istat = *ISTAT;

        rts = !rts;
        *SIO_CTRL = (!rts) << 5;
    }

    ExitCriticalSection();

    restoreTimer(timer, prevState);
    
    for (int i = 0; i<TICKS; i++) {
        struct TimerState s = states[i];
        printf("%d, counter: 0x%04x, mode: 0x%04x, target: 0x%04x, istat: 0x%04x\n", 
            s.gpio, 
            s.counter, 
            s.mode, 
            s.target,
            s.istat
        );
    }
}

void __attribute__((optimize("Os"))) testWhenCounterEqualsTarget() {
    const int timer = 0;
    const int target = 4;
    const uint16_t mode = 
                    /* No sync */
        (1 << 3) |  /* Reset counter to 0 after reaching target */
        (1 << 4) |  /* Target irq */
        (0 << 5) |  /* Overflow irq */
        (1 << 6) | /* repeatedly IRQ */
        (1 << 7) | /* toggle IRQ */
        (1 << 8)    /* dotclock */
    ;
    
    // Count from 0 to TARGET (included)
    printf("\nManually clock Timer%d counter using GPIO (RTS, CPU PIN 70).\n", timer);
    printf("Check how long it takes for counter to reset 0 after it hit the target\n");

    // Explain all bits in timer mode
    explain((CounterMode)mode);

    const uint32_t timerBase = 0x1f801100 + (timer * 0x10);
    uint16_t prevState = read16(timerBase + 4);
    
    EnterCriticalSection();

    bool rts = false;
    sioSetRts(rts);

    write16(timerBase + 8, target);
    write16(timerBase + 4, mode);
    write16(timerBase, 0); // Reset current value
    read16(timerBase + 4); // Reset reached bits

    DumpTimerValues();

    ExitCriticalSection();

    restoreTimer(timer, prevState);

    uint16_t* values = (uint16_t*)0x1F800000;
    int cnt = 0;
    printf("Target: 0x%04x\n", target);
    for (int i = 0; i<128; i++) {
        bool state = cnt < 4;
        printf("%3d, %d counter: 0x%04x\n", 
            i, state, values[i]
        );
        if (++cnt == 8) cnt = 0;

        if ((i+1) % 8 == 0) printf("\n");
    }
}

void runTests() {
    CounterMode mode(0);
    mode.resetToZero = CounterMode::ResetToZero::whenTarget;
    mode.irqWhenTarget = 1;
    mode.clockSource = (uint16_t)CounterMode::ClockSource0::dotClock;
    
    mode.irqPulseMode = CounterMode::IrqPulseMode::shortPulse;
    mode.irqRepeatMode = CounterMode::IrqRepeatMode::oneShot;
    testDumpTimerValuesCycleByCycle(mode);

    mode.irqPulseMode = CounterMode::IrqPulseMode::toggle;
    mode.irqRepeatMode = CounterMode::IrqRepeatMode::oneShot;
    testDumpTimerValuesCycleByCycle(mode);

    mode.irqPulseMode = CounterMode::IrqPulseMode::shortPulse;
    mode.irqRepeatMode = CounterMode::IrqRepeatMode::repeatedly;
    testDumpTimerValuesCycleByCycle(mode);

    mode.irqPulseMode = CounterMode::IrqPulseMode::toggle;
    mode.irqRepeatMode = CounterMode::IrqRepeatMode::repeatedly;
    testDumpTimerValuesCycleByCycle(mode);

    
    testWhenCounterEqualsTarget();
}

int main() {
    initVideo();
    SetVideoMode(MODE_NTSC);
  
    printf("timer-dump\n");
    printf("WARNING: This test requires you to have modified PSX motherboard with CPU PIN 160 (TCLK0) disconnected from GPU\n");
    printf("and connected to CPU PIN 70 (RTS), which allows this program for manual clocking of the Timer0 in Dot Clock mode.\n");
    sioInit();

    runTests();

    printf("\n\nDone.\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}