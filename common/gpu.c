#include "gpu.h"
#include <psxgpu.h>

DISPENV disp;
DRAWENV draw;

void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, w, h);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
}

void initVideo(int width, int height)
{
    ResetGraph(0);
    setResolution(width, height);
    SetDispMask(1);
}

void fillRect(int x, int y, int w, int h, int r, int g, int b) {
    FILL f;
    setFill(&f);
    setRGB0(&f, r, g, b);  
    setXY0(&f, x, y);
    setWH(&f, w, h);

    DrawPrim(&f);
}

void clearScreen() {
    fillRect(0,   0,   512, 256,   0, 0, 0);
    fillRect(512, 0,   512, 256,   0, 0, 0);
    fillRect(0,   256, 512, 256,   0, 0, 0);
    fillRect(512, 256, 0x3f1, 256, 0, 0, 0);
}

// TODO: Remove when #9 in PSn00bSDK is merged
#undef setDrawMask
#define setDrawMask( p, sb, mt ) \
	setlen( p, 1 ), (p)->code[0] = sb|(mt<<1), setcode( p, 0xe6 )

void setMaskBitSetting(bool setBit, bool checkBit) {
    DR_MASK mask;
    setDrawMask(&mask, setBit, checkBit);
    DrawPrim(&mask);
}

void gpuNop() {
    writeGP0(0, 0);
}

void writeGP0(uint8_t cmd, uint32_t value) {
    DR_TPAGE p;
    p.code[0] = value;
    setlen( &p, 1 );
    setcode( &p, cmd );
    DrawPrim(&p);
}

void writeGP1(uint8_t cmd, uint32_t data) {
    uint32_t *GP1 = (uint32_t*)0x1f801814;
    (*GP1) = (cmd << 24) | (data&0xffffff);
}

uint32_t readGPU() {
    uint32_t* GPUREAD = (uint32_t*)0x1f801810;
    return *GPUREAD;
}

void vramPut(int x, int y, uint16_t pixel) {
    CPU2VRAM buf = {0};
    setcode(&buf, 0xA0); // CPU -> VRAM
    setlen(&buf, 4);
    
    buf.x0 = x; // VRAM position
    buf.y0 = y;
    buf.w  = 1; // Transfer size - 1x1
    buf.h  = 1;
    buf.data = pixel; // pixel (lower 16bit)

    DrawPrim(&buf);
}

uint32_t vramGet(int x, int y) {
    VRAM2CPU buf = {0};
    setcode(&buf, 0xC0); // CPU -> VRAM
    setlen(&buf, 3);
    
    buf.x0 = x; // VRAM position
    buf.y0 = y;
    buf.w  = 1; // Transfer size - 1x1
    buf.h  = 1;

    DrawPrim(&buf);

    writeGP1(4, 3); // DMA Direction - VRAM -> CPU

    // Wait for VRAM to CPU ready
    while ((ReadGPUstat() & (1<<27)) == 0);

    return readGPU();
}
