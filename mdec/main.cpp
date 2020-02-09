#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <dma.hpp>
#include "vram.h"
#include "mdec.h"
#include "tables.h"
#include "log.h"

// NOTE: This test is not finished
// You can build it manually selecting 4/8/15/24 bit depths
// and MDECin/MDECout/GPU DMA or non-DMA

// #include "frame_chrono_350.h"
// uint8_t* frame   = frame_chrono_350;
// size_t frame_len = frame_chrono_350_len;

#include "frame_chrono_524.h"
uint8_t* frame   = frame_chrono_524;
size_t frame_len = frame_chrono_524_len;

#define SCR_W 320
#define SCR_H 240

#define USE_MDECIN_DMA  // Required for HW since I don't split transfer into blocks
// #define USE_MDECOUT_DMA // Works ok without it, but blocks aren't swizzled 
#define USE_GPU_DMA     // Works ok regardless
// #define COLOR_DEPTH 15

#ifndef COLOR_DEPTH
#error Please define COLOR_DEPTH to one or 4, 8, 15, 24 values
#endif

template <size_t pad = 0x20*4>
size_t getPadMdecFrameLen(size_t size) {
    size_t newSize = size;
    if ((newSize & (pad - 1)) != 0 ) {
        newSize &= ~(pad - 1);
        newSize += pad;
    }
    return newSize;
}

uint16_t* padMdecFrame(uint8_t* buf, size_t size) {
    size_t newSize = getPadMdecFrameLen(size);
    uint8_t* padded = (uint8_t*)malloc(newSize);

    uint8_t* ptr = padded;

    for (int i = 0; i<newSize - size; i+=2) {
        *ptr++ = 0x00;
        *ptr++ = 0xfe;
    }
    
    for (int i = 0; i<size; i++) {
        *ptr++ = *buf++;
    }

    return (uint16_t*)padded;
}

int main() {
    initVideo(SCR_W, SCR_H);
    printf("\nmdec\n");
#if COLOR_DEPTH == 24
    extern DISPENV disp;
    disp.isrgb24 = true;
    PutDispEnv(&disp);
    LOG("Using framebuffer in 24bit mode\n");
#else
    LOG("Using framebuffer in 15bit mode\n");
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
#elif COLOR_DEPTH == 8
    depth = ColorDepth::bit_8;
#elif COLOR_DEPTH == 4
    depth = ColorDepth::bit_4;
#endif

    size_t len = getPadMdecFrameLen(frame_len) / 2;
    uint16_t* padded = padMdecFrame(frame, frame_len);

#if defined(USE_MDECIN_DMA) || defined(USE_MDECOUT_DMA)
    mdec_enableDma();
#endif

#ifdef USE_MDECIN_DMA
    mdec_decodeDma((uint16_t*)padded, len, depth, false, true);
#else
    // TODO: It would need to run on separate thread 
    mdec_decode    ((uint16_t*)padded, len, depth, false, true);
#endif

#if COLOR_DEPTH == 24
    #define W_MULTIP 3 / 2
#elif COLOR_DEPTH == 15
    #define W_MULTIP 1
#elif COLOR_DEPTH == 8
    #define W_MULTIP 3 / 4
#elif COLOR_DEPTH == 4
    #define W_MULTIP 1 / 4
#endif
    const int FRAME_WIDTH = 320;
    const int FRAME_HEIGHT = 240;

    const int STRIPE_WIDTH = 16 * W_MULTIP;
    const int STRIPE_HEIGHT = FRAME_HEIGHT;

    // HW works with both DMA and software copy mdecout and gpu 
    
    uint32_t* buffer = (uint32_t*)malloc(((STRIPE_WIDTH * STRIPE_HEIGHT) * sizeof(uint32_t)));
    for (int c = 0; c < FRAME_WIDTH / 16; c++) {
#ifdef USE_MDECOUT_DMA
        mdec_readDma(buffer, STRIPE_WIDTH * STRIPE_HEIGHT / 2);
#else
        mdec_read    (buffer, STRIPE_WIDTH * STRIPE_HEIGHT / 2);
#endif
        
        writeGP0(1, 0);
#ifdef USE_GPU_DMA
        copyToVramDma(c * STRIPE_WIDTH, 0, STRIPE_WIDTH, STRIPE_HEIGHT, buffer);
#else
        copyToVram   (c * STRIPE_WIDTH, 0, STRIPE_WIDTH, STRIPE_HEIGHT, buffer);
#endif
    }

    free(buffer);
    free(padded);

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