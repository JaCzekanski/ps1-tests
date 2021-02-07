#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <dma.hpp>
#include <mdec.h>
#include <gpu.h>

// Frame is padded by mdec-tools
#include "sunset.h"
uint8_t* frame   = sunset_mdec;
size_t frame_len = sunset_mdec_len;

#define SCR_W 320
#define SCR_H 240

#define USE_MDECIN_DMA  // DMA required for now due to MDECin fifo size (32*uint32_t)
// #define USE_MDECOUT_DMA // Works ok with or without DMA (swizzling done in mdec_readDecoded)
#define USE_GPU_DMA     // Works ok with or without DMA

#ifndef COLOR_DEPTH
#error Please define COLOR_DEPTH as 15 or 24
#endif

int main() {
    SetVideoMode(MODE_NTSC);
    initVideo(SCR_W, SCR_H);
    printf("\nmdec/frame\n");
#if COLOR_DEPTH == 24
    extern DISPENV disp;
    disp.isrgb24 = true;
    PutDispEnv(&disp);
    printf("Using framebuffer in 24bit mode\n");
#else
    printf("Using framebuffer in 15bit mode\n");
#endif

    clearScreen();

    mdec_reset();
    mdec_quantTable(quant, true);
    mdec_idctTable((int16_t*)idct);

    ColorDepth depth;
#if COLOR_DEPTH == 24
    depth = ColorDepth::bit_24;
#elif COLOR_DEPTH == 15
    depth = ColorDepth::bit_15;
#endif
    
#if defined(USE_MDECIN_DMA) || defined(USE_MDECOUT_DMA)
    mdec_enableDma();
#endif

    const bool outputSigned = false;
    const bool setBit15 = false;
#ifdef USE_MDECIN_DMA
    mdec_decodeDma((uint16_t*)frame, frame_len, depth, outputSigned, setBit15);
#else
    // TODO: It would need to run on separate thread 
    mdec_decode    ((uint16_t*)frame, frame_len, depth, outputSigned, setBit15);
#endif

    // Single macroblock consists of 4 8x8 blocks
    // in 15bit mode single pixel is 2 bytes: 4 * 8 * 8 * 2 = 512  bytes = 128 uint32_t
    // in 25bit mode single pixel is 4 bytes: 4 * 8 * 8 * 4 = 1024 bytes = 256 uint32_t
#if COLOR_DEPTH == 24
    #define BYTES_PER_PIXEL 3
#elif COLOR_DEPTH == 15
    #define BYTES_PER_PIXEL 2
#endif
    const int FRAME_WIDTH = 320;
    const int FRAME_HEIGHT = 240;

    const int STRIPE_WIDTH = 16;
    const int STRIPE_HEIGHT = FRAME_HEIGHT;

    int copyWidth = STRIPE_WIDTH;
    if (depth == ColorDepth::bit_24) {
        copyWidth = STRIPE_WIDTH * 3/2;
    }

    uint32_t* buffer = (uint32_t*)malloc(STRIPE_WIDTH * STRIPE_HEIGHT * BYTES_PER_PIXEL / 4 * sizeof(uint32_t));
    for (int c = 0; c < FRAME_WIDTH / 16; c++) {
#ifdef USE_MDECOUT_DMA
        mdec_readDecodedDma(buffer, STRIPE_WIDTH * STRIPE_HEIGHT * BYTES_PER_PIXEL);
#else
        mdec_readDecoded   (buffer, STRIPE_WIDTH * STRIPE_HEIGHT * BYTES_PER_PIXEL);
#endif
        
        writeGP0(1, 0);
#ifdef USE_GPU_DMA
        vramWriteDMA(c * copyWidth, 0, copyWidth, STRIPE_HEIGHT, (uint16_t*)buffer);
#else
        vramWrite   (c * copyWidth, 0, copyWidth, STRIPE_HEIGHT, (uint16_t*)buffer);
#endif
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