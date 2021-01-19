#pragma once
#include "stdint.h"

void GTE_INIT();
uint32_t GTE_READ(uint8_t reg);
void GTE_WRITE(uint8_t reg, uint32_t value);
void _GTE_OP(uint32_t op);