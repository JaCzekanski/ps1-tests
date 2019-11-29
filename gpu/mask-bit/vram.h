#pragma once 
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void setMaskBitSetting(bool setBit, bool checkBit);
void vramPut(int x, int y, uint16_t pixel);
uint32_t vramGet(int x, int y) ;

#ifdef __cplusplus
}
#endif