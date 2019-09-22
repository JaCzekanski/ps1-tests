#ifndef GTE_H
#define GTE_H

#include "stdint.h"

union Command {
    struct _ {
        uint32_t cmd : 6;
        uint32_t : 4;
        uint32_t lm : 1;
        uint32_t : 2;
        uint32_t mvmvaTranslationVector : 2;  // 0 - tr, 1 - bk, 2 - fc, 3 - none
        uint32_t mvmvaMultiplyVector : 2;     // 0 - v0, 1 - v1, 2 - v2, 3 - ir
        uint32_t mvmvaMultiplyMatrix : 2;     // 0 - rotation, 1 - light, 2 - color, 3 - reserved
        uint32_t sf : 1;
        uint32_t : 5;
        uint32_t : 7;  // cop2
    } _;

    uint32_t _reg;
};

uint32_t GTE_READ(uint8_t reg);
void GTE_WRITE(uint8_t reg, uint32_t value);
void _GTE_OP(uint32_t op);

#endif