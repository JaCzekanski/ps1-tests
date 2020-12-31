#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <dma.hpp>
#include <mdec.h>
#include <gpu.h>

#include "bad_apple.h"

#define SCR_W 256
#define SCR_H 240

uint32_t* buffer = nullptr;
void decodeAndDisplayFrame(uint8_t* frame, size_t frameLen, uint16_t frameWidth, uint16_t frameHeight) {
    const int STRIPE_WIDTH = 16;
    const int STRIPE_HEIGHT = frameHeight;

    #ifdef USE_24BIT
    const ColorDepth depth = ColorDepth::bit_24;
    const int copyWidth = STRIPE_WIDTH * 3/2;
    #define BYTES_PER_PIXEL 3
    #else
    const ColorDepth depth = ColorDepth::bit_15;
    const int copyWidth = STRIPE_WIDTH;
    #define BYTES_PER_PIXEL 2
    #endif

    mdec_decodeDma((uint16_t*)frame, frameLen, depth, false, false);

    if (buffer == nullptr) {
        buffer = (uint32_t*)malloc(STRIPE_WIDTH * STRIPE_HEIGHT * BYTES_PER_PIXEL / 4 * sizeof(uint32_t));
    }
    for (int c = 0; c < frameWidth / STRIPE_WIDTH; c++) {
        mdec_readDecodedDma(buffer, STRIPE_WIDTH * STRIPE_HEIGHT * BYTES_PER_PIXEL);
        vramWriteDMA(c * copyWidth, 0, copyWidth, STRIPE_HEIGHT, (uint16_t*)buffer);
    }
}

int main() {
    SetVideoMode(MODE_NTSC);
    initVideo(SCR_W, SCR_H);
    printf("\nmdec/frame\n");
#ifdef USE_24BIT
    extern DISPENV disp;
    disp.isrgb24 = true;
    PutDispEnv(&disp);
    printf("Using framebuffer in 24bit mode\n");
#endif

    clearScreen();

    mdec_reset();
    mdec_quantTable(quant, true);
    mdec_idctTable((int16_t*)idct);    
    mdec_enableDma();

    for (int i = 0; i<60*2; i++) {
        VSync(0);
    }

    for (int i = 0; i<FRAME_COUNT; i++) {
        frame_t frame = frames[i];
        decodeAndDisplayFrame(&movieBitstream[frame.offset], frame.size, SCR_W, SCR_H);
        VSync(0);
    }

    free(buffer);
    printf("Done\n");

    // Stop all pending DMA transfers
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::MDECin, 0);
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::MDECout, 0);
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU, 0);
    DMA::masterEnable(DMA::Channel::MDECout, false);
    DMA::masterEnable(DMA::Channel::MDECin, false);
    DMA::masterEnable(DMA::Channel::GPU, false);

    mdec_reset();
    for (;;) {
        VSync(0);
    }
    return 0;
}