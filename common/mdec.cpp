#include <malloc.h>
#include "mdec.h"
#include "dma.hpp"
#include "io.h"
#include "log.h"

using namespace DMA;

int16_t idct[64] = {
	23170, 23170, 23170, 23170, 23170, 23170, 23170, 23170, 32138, 27245, 18204, 6392, -6393, -18205, -27246, -32139, 30273, 12539, -12540, -30274, -30274, -12540, 12539, 30273, 27245, -6393, -32139, -18205, 18204, 32138, 6392, -27246, 23170, -23171, -23171, 23170, 23170, -23171, -23171, 23170, 18204, -32139, 6392, 27245, -27246, -6393, 32138, -18205, 12539, -30274, 30273, -12540, -12540, 30273, -30274, 12539, 6392, -18205, 27245, -32139, 32138, -27246, 18204, -6393
};

uint8_t quant[128] = {
	// Luminance
	0x02, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x1a, 0x18, 0x1a, 0x1b, 0x1b, 0x1b, 0x1a, 0x1a, 0x1a, 0x1a, 0x1b, 0x1b, 0x1b, 0x1d, 0x1d, 0x1d, 0x22, 0x22, 0x22, 0x1d, 0x1d, 0x1d, 0x1b, 0x1b, 0x1d, 0x1d, 0x20, 0x20, 0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22, 0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30, 0x2e, 0x2e, 0x38, 0x38, 0x3a, 0x45, 0x45, 0x53, 
	// Chroma
	0x02, 0x10, 0x10, 0x13, 0x10, 0x13, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x1a, 0x18, 0x1a, 0x1b, 0x1b, 0x1b, 0x1a, 0x1a, 0x1a, 0x1a, 0x1b, 0x1b, 0x1b, 0x1d, 0x1d, 0x1d, 0x22, 0x22, 0x22, 0x1d, 0x1d, 0x1d, 0x1b, 0x1b, 0x1d, 0x1d, 0x20, 0x20, 0x22, 0x22, 0x25, 0x26, 0x25, 0x23, 0x23, 0x22, 0x23, 0x26, 0x26, 0x28, 0x28, 0x28, 0x30, 0x30, 0x2e, 0x2e, 0x38, 0x38, 0x3a, 0x45, 0x45, 0x53
};

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

void mdec_decode(const uint16_t* data, size_t lengthInBytes, enum ColorDepth colorDepth, bool outputSigned, bool setBit15) {
    LOG("mdec_decode(addr=0x%08x, length=0x%x, colorDepth=%d, outputSigned=%d, setBit15=%d)... \n", data, lengthInBytes, colorDepth, outputSigned, setBit15);
    
    size_t lengthWords = lengthInBytes / 4;
    uint32_t cmd = (1<<29) | (colorDepth<<27) | (outputSigned<<26) | (setBit15 << 25) | (lengthWords & 0xffff);

    while (mdec_cmdBusy());
    mdec_cmd(cmd);

    for (;lengthWords != 0;lengthWords--) {
        uint32_t word = *data++;
            word |= ((uint32_t)(*data++)) << 16;

        mdec_data(word);
        LOG("MDEC_DATA <- 0x%08x\n", word);
    }

    LOG("ok\n");
}

// Data should be 0x20 word block aligned and padded with 0xfe00
void mdec_decodeDma(const uint16_t* data, size_t lengthInBytes, enum ColorDepth colorDepth, bool outputSigned, bool setBit15) {
    LOG("mdec_decodeDma(addr=0x%08x, lengthInBytes=0x%x, colorDepth=%d, outputSigned=%d, setBit15=%d)... ", data, lengthInBytes, colorDepth, outputSigned, setBit15);

    size_t lengthWords = lengthInBytes / 4;
    uint32_t cmd = (1<<29) | (colorDepth<<27) | (outputSigned<<26) | (setBit15 << 25) | (lengthWords & 0xffff);

    while (mdec_cmdBusy());
    mdec_cmd(cmd);

    auto addr    = MADDR((uint32_t)data);
    const int BS = 0x20;
    auto block   = BCR::mode1(BS, (lengthWords) / BS);
    auto control = CHCR::MDECin();

    masterEnable(Channel::MDECin, true);
    waitForChannel(Channel::MDECin);
    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::MDECin, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::MDECin, block._reg);
    write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::MDECin, control._reg);

    LOG("ok\n");
}

void mdec_readDecoded(uint32_t* data, size_t bytes) {
    LOG("mdec_readDecoded(addr=0x%08x, bytes=0x%x)... ", data, bytes);

    while (mdec_dataOutFifoEmpty());
    
    int colorDepth = (read32(MDEC_STATUS) >> 25) & 3;
    
    if (colorDepth == ColorDepth::bit_15) { //15 bit
        // blocks are read from MDEC in this form:
        // 11111111 11111111
        // 11111111 11111111
        // 22222222 22222222
        // 22222222 22222222
        // 33333333 33333333
        // 33333333 33333333
        // 44444444 44444444
        // 44444444 44444444
        //
        // and before transfering them to GPU I need to swizzle them like that:
        // 11111111 22222222
        // 11111111 22222222
        // 11111111 22222222
        // 11111111 22222222
        // 33333333 44444444
        // 33333333 44444444
        // 33333333 44444444
        // 33333333 44444444

        uint16_t blocks[2][8][8];
        uint16_t* outPtr = (uint16_t*)data;
        auto writeBlocks = [&](){
            for (int y = 0; y < 8; y++) {
                for (int b = 0; b< 2; b++) {
                    for (int x = 0; x < 8; x++) {
                        *outPtr++ = blocks[b][y][x];
                    }   
                }
            }
        };
        
        int x = 0;
        int y = 0;
        int b = 0;
        for (int i = 0; i < bytes/4; i++) {
            uint32_t d = mdec_read();

            blocks[b][y][x+0] = d & 0xffff;
            blocks[b][y][x+1] = (d>>16) & 0xffff;

            x+=2;
            if (x >= 8) {
                x = 0;
                if (++y >= 8) {
                    y = 0;
                    if (++b == 2) {
                        b = 0;
                        writeBlocks();
                    }
                }
            }
        
            while (mdec_dataOutFifoEmpty());
        }
    } else {
        if (colorDepth == ColorDepth::bit_24) {
            int depth = 0;
            depth = 24;
            LOG("\nSwizzling for colorDepth %d not implemented...", depth);
        }
        for (int i = 0; i < bytes/4; i++) {
            *data++ = mdec_read();
        
            while (mdec_dataOutFifoEmpty());
        }
    }

    LOG("ok\n");
}

void mdec_readDecodedDma(uint32_t* data, size_t bytes) {
    // DMA seems to work fine with BS between 0x02 to 0x20 (must be divisible by 2)
    // TODO: Automatically find biggest BS <2..0x20> for current wordCount
    int BS = 0x20;
    LOG("mdec_readDecodedDma(addr=0x%08x, bytes=0x%x, [blockSize=0x%x])... ", data, bytes, BS);
    
    while (mdec_dataOutFifoEmpty());

    auto addr    = MADDR((uint32_t)data);
    auto block   = BCR::mode1(BS, (bytes/4) / BS);
    auto control = CHCR::MDECout();

    masterEnable(Channel::MDECout, true);
    waitForChannel(Channel::MDECout);
    
    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::MDECout, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::MDECout, block._reg);
    write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::MDECout, control._reg);

    volatile CHCR* ctrl = (CHCR*)(CH_CONTROL_ADDR + 0x10 * (int)Channel::MDECout);

    for (int i = 0; i<10001; i++) {
        if (ctrl->enabled == CHCR::Enabled::completed) {
            break;
        }
        if (i == 10000) {
            LOG("mdec_readDecodedDma timeout, CH_BASE_ADDR: 0x%08x, CH_BLOCK_ADDR: 0x%08x, CH_CONTROL_ADDR: 0x%08x\n ",
                read32(CH_BASE_ADDR    + 0x10 * (int)Channel::MDECout), 
                read32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::MDECout),
                read32(CH_CONTROL_ADDR + 0x10 * (int)Channel::MDECout)
            );
            break;
        }
    }

    LOG("ok\n");
}

size_t getPadMdecFrameLen(size_t size, size_t pad) {
    size_t newSize = size;
    if ((newSize & (pad - 1)) != 0 ) {
        newSize &= ~(pad - 1);
        newSize += pad;
    }
    return newSize;
}

uint8_t* padMdecFrame(uint8_t* buf, size_t size) {
    size_t newSize = getPadMdecFrameLen(size);
    uint8_t* padded = (uint8_t*)malloc(newSize);

    uint8_t* ptr = padded;


    for (int i = 0; i<size; i++) {
        *ptr++ = *buf++;
    }

    for (int i = 0; i<newSize - size; i+=2) {
        *ptr++ = 0x00;
        *ptr++ = 0xfe;
    }

    return padded;
}