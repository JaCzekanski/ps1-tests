#include "vram.h"
#include <psxgpu.h>

typedef struct CPU2VRAM {
	unsigned int	tag;
	unsigned char	p0,p1,p2,code;
	unsigned short	x0,y0;
	unsigned short	w,h;
	unsigned int	data; // Single pixel
} CPU2VRAM;

typedef struct VRAM2CPU {
	unsigned int	tag;
	unsigned char	p0,p1,p2,code;
	unsigned short	x0,y0;
	unsigned short	w,h;
} VRAM2CPU;

uint32_t ReadGPUstat();

// TODO: Remove when #9 in PSn00bSDK is merged
#undef setDrawMask
#define setDrawMask( p, sb, mt ) \
	setlen( p, 1 ), (p)->code[0] = sb|(mt<<1), setcode( p, 0xe6 )

void setMaskBitSetting(bool setBit, bool checkBit) {
    DR_MASK mask;
    setDrawMask(&mask, setBit, checkBit);
    DrawPrim(&mask);
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
