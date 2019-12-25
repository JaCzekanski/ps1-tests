#include <psx.h>
#include <stdio.h>
#include <stdlib.h>
#include "mdec.h"
#include "io.h"

void mdec_reset() {
    write32(MDEC_CTRL, 1<<31);
}

bool mdec_dataFifoEmpty() {
    return read32(MDEC_STATUS) & (1<<31);
}

bool mdec_cmdFifoFull() {
    return read32(MDEC_STATUS) & (1<<30);
}

void mdec_cmd(uint32_t cmd) {
    while (mdec_cmdFifoFull());
    write32(MDEC_CMD, cmd);
}

uint32_t mdec_read() {
    while (mdec_dataFifoEmpty());

    return read32(MDEC_DATA);
}

void mdec_quantTable(const uint8_t* table, bool color) {
    mdec_cmd((2<<29) | color);

    uint8_t count = color?32:16;

    for (uint8_t i = 0; i<count; i++) {
        uint32_t arg = 0;
        arg |= table[i * 4 + 0] << 0;
        arg |= table[i * 4 + 1] << 8;
        arg |= table[i * 4 + 2] << 16;
        arg |= table[i * 4 + 3] << 24;

        mdec_cmd(arg);
    }
}

void mdec_idctTable(const int16_t* table) {
    mdec_cmd((3<<29));

    for (uint8_t i = 0; i<64 / 2; i++) {
        uint32_t arg = 0;
        arg |= (uint16_t)table[i * 2 + 0] << 0;
        arg |= (uint16_t)table[i * 2 + 1] << 16;

        mdec_cmd(arg);
    }
}

void mdec_decode(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15) {
    size_t lengthWords = (length + 1) / 2;
    uint32_t cmd = (1<<29) | (colorDepth<<27) | (outputSigned<<26) | (setBit15 << 25) | (lengthWords & 0xffff);

    mdec_cmd(cmd);

    for (;lengthWords != 0;lengthWords--) {
        uint32_t word = *data++;

        if (length > 0) {
            word |= ((uint32_t)(*data++)) << 16;
        } else {
            word |= 0xfe00 << 16;
        }

        mdec_cmd(word);
    }
}