#pragma once
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MDEC_BASE 0x1f801820
#define MDEC_CMD    (MDEC_BASE + 0)
#define MDEC_DATA   (MDEC_BASE + 0)
#define MDEC_CTRL   (MDEC_BASE + 4)
#define MDEC_STATUS (MDEC_BASE + 4)

enum ColorDepth { bit_4 = 0, bit_8 = 1, bit_24 = 2, bit_15 = 3 };

extern int16_t idct[64];
extern uint8_t quant[128];

// Low level functions
void mdec_reset();
void mdec_enableDma();
bool mdec_dataOutFifoEmpty();
bool mdec_dataInFifoFull();
bool mdec_cmdFifoFull();
bool mdec_cmdBusy();
void mdec_cmd(uint32_t cmd);
void mdec_ctrl(uint32_t ctrl);
uint32_t mdec_read();
void mdec_quantTable(const uint8_t* table, bool color);
void mdec_idctTable(const int16_t* table);
void mdec_decode(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15);
void mdec_decodeDma(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15);
void mdec_readDecoded(uint32_t* data, size_t wordCount);
void mdec_readDecodedDma(uint32_t* data, size_t wordCount);

size_t getPadMdecFrameLen(size_t size, size_t pad = 0x20 * 4);
uint8_t* padMdecFrame(uint8_t* buf, size_t size);

#ifdef __cplusplus
}
#endif
