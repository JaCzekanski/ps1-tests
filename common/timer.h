#pragma once
#include <io.h>

#define TIMER_BASE(t) (0x1f801100 + ((t) * 0x10))

inline uint16_t initTimer(int timer, int mode) {
    uint16_t prevMode = read16(TIMER_BASE(timer) + 4);
    write16(TIMER_BASE(timer) + 4, mode << 8);
    return prevMode;
}

inline void restoreTimer(int timer, uint16_t prevMode) {
    write16(TIMER_BASE(timer) + 4, prevMode);
}

inline uint16_t readTimer(int timer) {
    return read16(TIMER_BASE(timer));
}

inline bool timerDidOverflow(int timer) {
    return (read16(TIMER_BASE(timer) + 4) & (1<<12)) != 0;
}

inline void resetTimer(int timer) {
    read16(TIMER_BASE(timer) + 4); // reset overflow flag
    write16(TIMER_BASE(timer), 0);
}

#undef TIMER_BASE