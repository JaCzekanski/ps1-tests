#include "mdec.h"
#include <common.h>
#include <dma.hpp>
#include <io.h>
#include "log.h"

using namespace DMA;

void mdec_reset() {
    uint32_t value = 1<<31;
    
    LOG("mdec_reset (MDEC_CTRL <- 0x%08x)... ", value);
    mdec_ctrl(value);
    LOG("ok\n");
}

void mdec_enableDma() {
    uint32_t value = (1<<30) | (1<<29);

    LOG("mdec_enableDma (MDEC_CTRL <- 0x%08x)... ", value);
    mdec_ctrl(value);
    LOG("ok\n");
}

bool mdec_dataOutFifoEmpty() {
    return read32(MDEC_STATUS) & (1<<31);
}

bool mdec_dataInFifoFull() {
    return read32(MDEC_STATUS) & (1<<30);
}

bool mdec_cmdBusy() {
    return read32(MDEC_STATUS) & (1<<29);
}

void mdec_cmd(uint32_t cmd) {
    while (mdec_cmdBusy());
    write32(MDEC_CMD, cmd);
}

void mdec_data(uint32_t cmd) {
    while (mdec_dataInFifoFull());
    write32(MDEC_CMD, cmd);
}

uint32_t mdec_read() {
    while (mdec_dataOutFifoEmpty());
    return read32(MDEC_DATA);
}

void mdec_ctrl(uint32_t ctrl) {
    write32(MDEC_CTRL, ctrl);
}

void mdec_quantTable(const uint8_t* table, bool color) {
    LOG("mdec_quantTable(addr=0x%08x, color=%d)... ", table, color);
    mdec_cmd((2<<29) | color);

    uint8_t count = color?32:16;

    for (uint8_t i = 0; i<count; i++) {
        uint32_t arg = 0;
        arg |= table[i * 4 + 0] << 0;
        arg |= table[i * 4 + 1] << 8;
        arg |= table[i * 4 + 2] << 16;
        arg |= table[i * 4 + 3] << 24;

        mdec_data(arg);
    }
    LOG("ok\n");
}

void mdec_idctTable(const int16_t* table) {
    LOG("mdec_idctTable(addr=0x%08x)... ", table);
    mdec_cmd((3<<29));

    for (uint8_t i = 0; i < 64 / 2; i++) {
        uint32_t arg = 0;
        arg |= (uint16_t)table[i * 2 + 0] << 0;
        arg |= (uint16_t)table[i * 2 + 1] << 16;

        mdec_data(arg);
    }
    LOG("ok\n");
}

void mdec_decode(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15) {
    LOG("mdec_decode(addr=0x%08x, length=0x%x, colorDepth=%d, outputSigned=%d, setBit15=%d)... ", data, length, colorDepth, outputSigned, setBit15);
    
    size_t lengthWords = (length) / 2;
    uint32_t cmd = (1<<29) | (colorDepth<<27) | (outputSigned<<26) | (setBit15 << 25) | (lengthWords & 0xffff);

    while (mdec_cmdBusy());
    mdec_cmd(cmd);

    for (;lengthWords != 0;lengthWords--) {
        uint32_t word = *data++;

        // if (lengthWords > 0) {
            word |= ((uint32_t)(*data++)) << 16;
        // } else {
        //     word |= 0xfe00 << 16;
        // }

        mdec_data(word);
    }

    LOG("ok\n");
}

// Data should be 0x20 word block aligned and padded with 0xfe00
void mdec_decodeDma(const uint16_t* data, size_t length, enum ColorDepth colorDepth, bool outputSigned, bool setBit15) {
    LOG("mdec_decodeDma(addr=0x%08x, length=0x%x, colorDepth=%d, outputSigned=%d, setBit15=%d)... ", data, length, colorDepth, outputSigned, setBit15);

    size_t lengthWords = (length) / 2;
    uint32_t cmd = (1<<29) | (colorDepth<<27) | (outputSigned<<26) | (setBit15 << 25) | (lengthWords & 0xffff);

    while (mdec_cmdBusy());
    mdec_cmd(cmd);

    // TODO: This isn't valid implementation, check in docs how to do it
    // int padding = lengthWords % 0x20;
    // for (int i = 0; i < padding; i++) {
    //     mdec_data(0xfe00fe00);
    // }

    // lengthWords -= padding;

    // TODO: DMA is missing last termination word

    auto addr    = MADDR((uint32_t)data);
    auto block   = BCR::mode1(0x20, (lengthWords) / 0x20);
    auto control = CHCR::MDECin();

    masterEnable(Channel::MDECin, true);
    waitForChannel(Channel::MDECin);
    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::MDECin, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::MDECin, block._reg);
    write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::MDECin, control._reg);

    LOG("ok\n");
}

void mdec_read(uint32_t* data, size_t wordCount) {
    LOG("mdec_read(addr=0x%08x, wordCount=0x%x... ", data, wordCount);
    
    while (mdec_dataOutFifoEmpty());

    for (int i = 0; i < wordCount; i++) {
        *data++ = mdec_read();
    }

    LOG("ok\n");
}

void mdec_readDma(uint32_t* data, size_t wordCount) {
    LOG("mdec_readDma(addr=0x%08x, wordCount=0x%x... ", data, wordCount);
    
    while (mdec_dataOutFifoEmpty());

    auto addr    = MADDR((uint32_t)data);
    auto block   = BCR::mode1(0x20, wordCount / 0x20);
    auto control = CHCR::MDECout();

    masterEnable(Channel::MDECout, true);
    waitForChannel(Channel::MDECout);
    
    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::MDECout, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::MDECout, block._reg);
    write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::MDECout, control._reg);

    waitForChannel(Channel::MDECout);

    LOG("ok\n");
}