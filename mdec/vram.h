#pragma once
#include <stdint.h>

void copyToVram(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t* data);
void copyToVramDma(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t* data);