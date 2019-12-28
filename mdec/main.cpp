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

// #include "frame_chrono_350.h"
// uint8_t* frame   = frame_chrono_350;
// size_t frame_len = frame_chrono_350_len;

#include "frame_chrono_524.h"
uint8_t* frame   = frame_chrono_524;
size_t frame_len = frame_chrono_524_len;

#define SCR_W 320
#define SCR_H 240

#define USE_DMA 
// #define USE_24_BIT

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
#ifdef USE_24_BIT
    extern DISPENV disp;
    disp.isrgb24 = true;
    PutDispEnv(&disp);
    LOG("Using 24bit mode\n");
#else
    LOG("Using 15bit mode\n");
#endif

    clearScreen();

    mdec_reset();
    mdec_quantTable(quant, true);
    mdec_idctTable((int16_t*)idct);

    ColorDepth depth;
#ifdef USE_24_BIT
    depth = ColorDepth::bit_24;
#else 
    depth = ColorDepth::bit_15;
#endif

    size_t len = getPadMdecFrameLen(frame_len) / 2;
    uint16_t* padded = padMdecFrame(frame, frame_len);

#ifdef USE_DMA
    mdec_enableDma();
    mdec_decodeDma((uint16_t*)padded, len, depth, false, true);
#else
    mdec_decode    ((uint16_t*)padded, len, depth, false, true);
#endif

#ifdef USE_24_BIT
    #define W_MULTIP 3 / 2
#else
    #define W_MULTIP 1
#endif
    const int FRAME_WIDTH = 320;
    const int FRAME_HEIGHT = 240;

    const int STRIPE_WIDTH = 16 * W_MULTIP;
    const int STRIPE_HEIGHT = FRAME_HEIGHT;

    // HW works with both DMA and software copy mdecout and gpu 
    
    uint32_t* buffer = (uint32_t*)malloc(((STRIPE_WIDTH * STRIPE_HEIGHT) * sizeof(uint32_t)));
    for (int c = 0; c < FRAME_WIDTH / 16; c++) {
#ifdef USE_DMA
        mdec_readDma(buffer, STRIPE_WIDTH * STRIPE_HEIGHT / 2);
#else
        mdec_read    (buffer, STRIPE_WIDTH * STRIPE_HEIGHT / 2);
#endif
        
        writeGP0(1, 0);
#ifdef USE_DMA
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