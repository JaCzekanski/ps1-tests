#pragma once
#include "stdint.h"
#include <psxgpu.h>

#ifdef __cplusplus
extern "C" {
#endif

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

void setResolution(int w, int h);
void initVideo(int width, int height);
void fillRect(int x, int y, int w, int h, int r, int g, int b);
void clearScreen();

void setMaskBitSetting(bool setBit, bool checkBit);
void gpuNop();
uint32_t ReadGPUstat();
void writeGP0(uint8_t cmd, uint32_t value);
void writeGP1(uint8_t cmd, uint32_t data);

uint32_t readGPU();
void vramPut(int x, int y, uint16_t pixel);
uint32_t vramGet(int x, int y);

#ifdef __cplusplus
}
#endif