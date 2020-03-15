#include "timer.h"
#include <psxapi.h>

enum class MeasureMethod {
    CpuClock,
    CpuClock8,
    HBlank
};

// CpuClock - measure in cpu cycles
// CpuClock8 - measure in cpu cycles, granularity is 8 cycles
// HBlank - measure in hblanks
template <MeasureMethod method = MeasureMethod::CpuClock8>
uint32_t measure(void (*func)()) {
    int TIMER;
    if      constexpr (method == MeasureMethod::CpuClock)  TIMER = 2;
    else if constexpr (method == MeasureMethod::CpuClock8) TIMER = 2;
    else if constexpr (method == MeasureMethod::HBlank)    TIMER = 1;

    int CLOCK_SRC;
    if      constexpr (method == MeasureMethod::CpuClock)  CLOCK_SRC = 0;
    else if constexpr (method == MeasureMethod::CpuClock8) CLOCK_SRC = 2;
    else if constexpr (method == MeasureMethod::HBlank)    CLOCK_SRC = 1;

    EnterCriticalSection();
    uint16_t prevMode = initTimer(TIMER, CLOCK_SRC); 
    uint32_t timerValue = 0;
    
    {
        resetTimer(TIMER);
    
        func();

        timerValue = readTimer(TIMER);
        if (timerDidOverflow(TIMER)) timerValue += 0xffff;
    }

    restoreTimer(TIMER, prevMode);
    ExitCriticalSection();

    if constexpr (method == MeasureMethod::CpuClock8) timerValue *= 8;
    return timerValue;
}