#include "mdec-tools.h"
#include "gpu.h"

void copy4bitBlockToVram(uint8_t* src, int dstX, int dstY) {
    const int W = 8, H = 8;
    uint16_t out[8][8];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            uint8_t nibble = src[y*W/2 + x/2] >> ((x%2)*4);
            uint8_t p = (nibble & 0xf) << 1;
            out[y][x] = p | (p<<5) | (p<<10);
        }
    }
    vramWriteDMA(dstX, dstY, W, H, (uint16_t*)out);
};

void copy8bitBlockToVram(uint8_t* src, int dstX, int dstY) {
    const int W = 8, H = 8;
    uint16_t out[8][8];
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            uint8_t p = src[y*W + x] >> 3;
            out[y][x] = p | (p<<5) | (p<<10);
        }
    }
    vramWriteDMA(dstX, dstY, W, H, (uint16_t*)out);
};
