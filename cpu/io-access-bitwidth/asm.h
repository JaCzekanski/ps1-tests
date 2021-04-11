#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void asm_write_32(uint32_t address, uint32_t data);
void asm_write_16(uint32_t address, uint32_t data);
void asm_write_8(uint32_t address, uint32_t data);

#ifdef __cplusplus
}
#endif