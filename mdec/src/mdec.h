#include <psx.h>
#include <stdio.h>
#include <stdlib.h>
#include "io.h"

#define MDEC_BASE 0x1f801820
#define MDEC_CMD    (MDEC_BASE + 0)
#define MDEC_DATA   (MDEC_BASE + 0)
#define MDEC_CTRL   (MDEC_BASE + 4)
#define MDEC_STATUS (MDEC_BASE + 4)

enum ColorDepth { bit_4, bit_8, bit_24, bit_15 };

void mdec_reset();
bool mdec_dataFifoEmpty();
bool mdec_cmdFifoFull();
void mdec_cmd(uint32_t cmd);
uint32_t mdec_read();
void mdec_quantTable(const uint8_t* table, bool color);
void mdec_idctTable(const int16_t* table);
void mdec_decode(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15);