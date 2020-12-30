#pragma once
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

void hexdump(uint8_t* data, size_t length, size_t W = 16, bool ascii = false);

#ifdef __cplusplus
}
#endif