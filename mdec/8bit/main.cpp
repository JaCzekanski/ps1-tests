#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <dma.hpp>
#include <mdec.h>
#include <mdec-tools.h>
#include <gpu.h>
#include <hexdump.h>
#include "heart.h"

int main() {
    initVideo(320, 240);
    printf("\nmdec/8bit (single 8x8 block decode in 8-bit mode)\n");
    clearScreen();

    mdec_reset();
    mdec_quantTable(quant, true);
    mdec_idctTable(idct);
    mdec_enableDma();

    printf("8x8 block in 8bit mode:\n");

    // setBit15 - doesn't affect 8bit mode
    // outputSigned - does affect 8bit
    mdec_decodeDma((uint16_t*)heart_mdec, heart_mdec_len, ColorDepth::bit_8, false, false);

    const int W = 8, H = 8;
    uint8_t buffer[H][W];
    mdec_readDecodedDma((uint32_t*)buffer, W*H);

    hexdump((uint8_t*)buffer, sizeof(buffer), 8);

    copy8bitBlockToVram((uint8_t*) buffer, 32, 32);

    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::MDECin, 0);
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::MDECout, 0);
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU, 0);
    DMA::masterEnable(DMA::Channel::MDECout, false);
    DMA::masterEnable(DMA::Channel::MDECin, false);
    DMA::masterEnable(DMA::Channel::GPU, false);

    mdec_reset();
    printf("\nDone\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}