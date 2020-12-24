#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <delay.h>
#include <io.h>
#include <dma.hpp>
#include "vram.h"
#include "mdec.h"
#include "log.h"

void hexdump(uint8_t* data, size_t length, size_t W = 16) {
    const int H = length / W;
    for (int y = 0; y<H; y++) {
        printf("%02x: ", y*W);
        for (int x = 0; x<W; x++) {
            if (y*W+x >= length) break;
            printf("%02x ", data[y * W + x]);
        }
        printf("\n");
    }
}

void setTexPage(int x, int y, ColorDepth depth) {
    int bits = 0;
    if (depth == ColorDepth::bit_4) bits = 0;
    else if (depth == ColorDepth::bit_8) bits = 1;
    else if (depth == ColorDepth::bit_15) bits = 2;
    DR_TPAGE e;
    unsigned short texpage = getTPage(bits, /* semi-transparency mode */ 0, x, y);
    setDrawTPage(&e, /* Drawing to display area */ 1, /* dithering */ 0, texpage);
    DrawPrim(&e);
}

void drawRect(int x, int y, int w, int h, int u, int v, int clutX, int clutY, ColorDepth depth) {
    // LOG("drawRect(x=%3d, y=%3d, w=%3d, h=%3d, u=%3d, v=%3d, clutX=%3d, clutY=%3d, depth=%3d)... ", x, y, w, h, u, v, clutX, clutY, depth);
    writeGP0(1, 0);
    setTexPage(u, v, depth);
    
    SPRT s;
    setSprt(&s);
    setSemiTrans(&s, 0);
    setXY0(&s, x, y);
    setWH(&s, w, h);
    setRGB0(&s, 0x80, 0x80, 0x80);
    setShadeTex(&s, 0); // raw texture, no blending
    setClut(&s, clutX, clutY);
    setUV0(&s, u % 64, v % 256);

    DrawPrim(&s);
    // LOG("ok\n");
}

// NOTE: This test is not finished
// You can build it manually selecting 4/8/15/24 bit depths
// and MDECin/MDECout/GPU DMA or non-DMA

#include "frame_chrono_350.h"
uint8_t* frame   = frame_chrono_350;
size_t frame_len = frame_chrono_350_len;

// #include "frame_chrono_524.h"
// uint8_t* frame   = frame_chrono_524;
// size_t frame_len = frame_chrono_524_len;

// #include "sunset.h"
// uint8_t* frame   = sunset_mdec;
// size_t frame_len = sunset_mdec_len;

#define SCR_W 320
#define SCR_H 240

#define USE_MDECIN_DMA  // Required for HW since I don't split transfer into blocks
#define USE_MDECOUT_DMA // Works ok without it, but blocks aren't swizzled 
#define USE_GPU_DMA     // Works ok regardless, Broken on avocado?

#ifndef COLOR_DEPTH
#error Please define COLOR_DEPTH to one or 4, 8, 15, 24 values
#endif

int main() {
    SetVideoMode(MODE_NTSC);
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
    
    size_t len = getPadMdecFrameLen(frame_len);
    uint8_t* padded = padMdecFrame(frame, frame_len);

#if defined(USE_MDECIN_DMA) || defined(USE_MDECOUT_DMA)
    mdec_enableDma();
#endif

    const bool outputSigned = false;
    const bool setBit15 = false;
#ifdef USE_MDECIN_DMA
    mdec_decodeDma((uint16_t*)padded, len, depth, outputSigned, setBit15);
#else
    // TODO: It would need to run on separate thread 
    mdec_decode    ((uint16_t*)padded, len, depth, outputSigned, setBit15);
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
        mdec_readDecodedDma(buffer, STRIPE_WIDTH * STRIPE_HEIGHT * 2);
#else
        mdec_readDecoded   (buffer, STRIPE_WIDTH * STRIPE_HEIGHT * 2);
#endif
        
        writeGP0(1, 0);
#ifdef USE_GPU_DMA
        copyToVramDma(c * STRIPE_WIDTH, 0, STRIPE_WIDTH, STRIPE_HEIGHT, buffer);
#else
        copyToVram   (c * STRIPE_WIDTH, 0, STRIPE_WIDTH, STRIPE_HEIGHT, buffer);
#endif

        // if (c==0) hexdump((uint8_t*)buffer, (STRIPE_WIDTH * STRIPE_HEIGHT) * sizeof(uint32_t));
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
    printf("Done\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}