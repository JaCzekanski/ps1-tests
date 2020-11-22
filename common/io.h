#pragma once
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

inline void nop(void) {
  __asm__ __volatile__ ("nop");
}

inline void brk(void) {
  __asm__ __volatile__ ("break");
}

inline void softRestart(void) {
  __asm__ volatile ("lui $t0, 0xBFC0");
  __asm__ volatile ("jr $t0");
  __asm__ volatile ("nop");
}

uint32_t read32(uint32_t addr);
uint16_t read16(uint32_t addr);
uint8_t read8(uint32_t addr);
void write32(uint32_t addr, uint32_t val);
void write16(uint32_t addr, uint16_t val);
void write8(uint32_t addr, uint8_t val);

#ifdef __cplusplus
}
#endif
