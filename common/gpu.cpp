#include "gpu.h"
#include "dma.hpp"
#include "io.h"
#include <psxgpu.h>

DISPENV disp;
DRAWENV draw;

static void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, 1024, 512);

    if (h == 480) {
        disp.isinter = true; // Interlace mode
	    draw.dfe = true; // Drawing to display area (odd and even lines)
    }

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

void clearScreenColor(uint8_t r, uint8_t g, uint8_t b) {
    fillRect(0,   0,   512, 256,   r, g, b);
    fillRect(512, 0,   512, 256,   r, g, b);
    fillRect(0,   256, 512, 256,   r, g, b);
    fillRect(512, 256, 0x3f1, 256, r, g, b);
}

void clearScreen() {
    clearScreenColor(0, 0, 0);
}

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
    CPU2VRAM buf;
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
    VRAM2CPU buf;
    setcode(&buf, 0xC0); // VRAM -> CPU
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

void vramWriteDMA(int x, int y, int w, int h, uint16_t* ptr) {
    CPU2VRAM buf;
    setcode(&buf, 0xA0); // CPU -> VRAM
    setlen(&buf, 3);
    
    buf.x0 = x;
    buf.y0 = y;
    buf.w  = w; 
    buf.h  = h;

    DrawPrim(&buf);
    
    writeGP1(4, 2); // DMA Direction - CPU -> VRAM
    
    using namespace DMA;
    DMA::masterEnable(Channel::GPU, true);
    DMA::waitForChannel(Channel::GPU);
  
    write32(baseAddr(Channel::GPU),    MADDR((uint32_t)ptr)._reg);
    write32(blockAddr(Channel::GPU),   BCR::mode1(0x10, w * h / 0x10 / 2)._reg);
    write32(controlAddr(Channel::GPU), CHCR::VRAMwrite()._reg);
    
    DMA::waitForChannel(Channel::GPU);
}

void vramReadDMA(int x, int y, int w, int h, uint16_t* ptr) {
    VRAM2CPU buf;
    setcode(&buf, 0xC0); // VRAM -> CPU
    setlen(&buf, 3);
    
    buf.x0 = x;
    buf.y0 = y;
    buf.w  = w; 
    buf.h  = h;

    DrawPrim(&buf);

    writeGP1(4, 3); // DMA Direction - VRAM -> CPU

    // Wait for VRAM to CPU ready
    while ((ReadGPUstat() & (1<<27)) == 0);

    using namespace DMA;
    DMA::masterEnable(Channel::GPU, true);
    DMA::waitForChannel(Channel::GPU);
  
    write32(baseAddr(Channel::GPU),    MADDR((uint32_t)ptr)._reg);
    write32(blockAddr(Channel::GPU),   BCR::mode1(0x10, w * h / 0x10 / 2)._reg);
    write32(controlAddr(Channel::GPU), CHCR::VRAMread()._reg);
    
    DMA::waitForChannel(Channel::GPU);
}

void vramToVramCopy(int srcX, int srcY, int dstX, int dstY, int w, int h) 
{
    VRAM2VRAM buf;
    setcode(&buf, 0x80); // VRAM -> VRAM
    setlen(&buf, 4);
    
    buf.x0 = srcX;
    buf.y0 = srcY;
    buf.x1 = dstX;
    buf.y1 = dstY;
    buf.w = w;
    buf.h = h;

    DrawPrim(&buf);
}
