#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void FlushCache();
uint32_t codeRam();
uint32_t codeScratchpad();

#ifdef __cplusplus
}
#endif