#pragma once
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

// Convert 8x8 monochrome 4/8bit image to 15bit native pixfmt and copy it to VRAM 
void copy4bitBlockToVram(uint8_t* src, int dstX, int dstY);
void copy8bitBlockToVram(uint8_t* src, int dstX, int dstY);

#ifdef __cplusplus
}
#endif
