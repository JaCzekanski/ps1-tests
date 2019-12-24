#pragma once
#include "stdint.h"

static inline void nop(void) {
  __asm__ __volatile__ ("nop");
}

static inline void brk(void) {
  __asm__ __volatile__ ("break");
}

static inline void softRestart(void) {
  __asm__ volatile ("lui $t0, 0xBFC0");
  __asm__ volatile ("jr $t0");
  __asm__ volatile ("nop");
}

static inline uint32_t read32(size_t addr) {
  volatile const uint32_t *p = (volatile uint32_t *)addr;

  return *p;
}

static inline uint16_t read16(size_t addr) {
  volatile const uint16_t *p = (volatile uint16_t *)addr;

  return *p;
}

static inline uint8_t read8(size_t addr) {
  volatile const uint8_t *p = (volatile uint8_t *)addr;

  return *p;
}

static inline void write32(size_t addr, uint32_t val) {
  volatile uint32_t *p = (volatile uint32_t *)addr;

  *p = val;
}

static inline void write16(size_t addr, uint16_t val) {
  volatile uint16_t *p = (volatile uint16_t *)addr;

  *p = val;
}

static inline void write8(size_t addr, uint8_t val) {
  volatile uint8_t *p = (volatile uint8_t *)addr;

  *p = val;
}