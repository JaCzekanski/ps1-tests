#include "io.h"

uint32_t read32(uint32_t addr) {
  return *((volatile const uint32_t*)addr);
}

uint16_t read16(uint32_t addr) {
  return *((volatile const uint16_t*)addr);
}

uint8_t read8(uint32_t addr) {
  return *((volatile const uint8_t*)addr);
}

void write32(uint32_t addr, uint32_t val) {
  *((volatile uint32_t*)addr) = val;
}

void write16(uint32_t addr, uint16_t val) {
  *((volatile uint16_t*)addr) = val;
}

void write8(uint32_t addr, uint8_t val) {
  *((volatile uint8_t*)addr) = val;
}