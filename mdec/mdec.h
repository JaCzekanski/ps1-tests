#pragma once
#include <stdint.h>

#define MDEC_BASE 0x1f801820
#define MDEC_CMD    (MDEC_BASE + 0)
#define MDEC_DATA   (MDEC_BASE + 0)
#define MDEC_CTRL   (MDEC_BASE + 4)
#define MDEC_STATUS (MDEC_BASE + 4)

enum ColorDepth { bit_4 = 0, bit_8 = 1, bit_24 = 2, bit_15 = 3 };

void mdec_reset();
void mdec_enableDma();
bool mdec_dataFifoEmpty();
bool mdec_cmdFifoFull();
void mdec_cmd(uint32_t cmd);
void mdec_ctrl(uint32_t ctrl);
uint32_t mdec_read();
void mdec_quantTable(const uint8_t* table, bool color);
void mdec_idctTable(const int16_t* table);
void mdec_decode(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15);
void mdec_decodeDma(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15);
void mdec_read(uint32_t* data, size_t wordCount);
void mdec_readDma(uint32_t* data, size_t wordCount);