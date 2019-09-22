#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxspu.h>
#include "voice_1_left.h"
#include "voice_1_right.h"
#include "voice_2_left.h"
#include "voice_2_right.h"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

DISPENV disp;
DRAWENV draw;

uint32_t spuAddress = 0x1000;
 
void setResolution(int w, int h) {
    SetDefDispEnv(&disp, 0, 0, w, h);
    SetDefDrawEnv(&draw, 0, 0, w, h);

    draw.isbg = 1;
    setRGB0(&draw, 0, 0, 0);

    PutDispEnv(&disp);
    PutDrawEnv(&draw);
}

void initVideo()
{
    ResetGraph(0);
    setResolution(320, 240);
    SetDispMask(1);
}

uint32_t uploadToSpuRam(unsigned char buf[], unsigned size) {
    uint32_t ptr = spuAddress; 
    spuAddress += size;

    SpuSetTransferStartAddr(ptr);

    int dataOffset = 64;
    SpuWrite(buf+dataOffset, size - dataOffset);
    SpuWait();

    printf("[SPU] Upload %d bytes to SPU RAM (sample at 0x%x)\n", size, ptr);

    return ptr;
}

void play(uint32_t ptr, int voice, uint16_t leftVolume, uint16_t rightVolume) {
    SpuVoiceRaw v;
    v.vol.left = leftVolume;
    v.vol.right = rightVolume;
    v.freq = 22050/10;
    v.addr = ptr/8;
    v.adsr_param = 0;

    printf("Voice %d (L: 0x%04x, R: 0x%04x)\n", voice + 1, leftVolume, rightVolume);
    SpuSetVoiceRaw(voice, &v);
    SpuSetKey(1, SPU_KEYCH(voice));
}

void delay(int frames) {
    for (int i = 0; i<frames; i++) VSync(0);
}

#define UPLOAD(addr) uploadToSpuRam((addr), sizeof(addr)/sizeof(unsigned char))

int main()
{
    printf("spu/stereo\n");
    initVideo();

    SpuInit();
    SpuSetTransferMode(4); // ?

    uint16_t v1l = UPLOAD(voice_1_left_vag);
    uint16_t v1r = UPLOAD(voice_1_right_vag);
    uint16_t v2l = UPLOAD(voice_2_left_vag);
    uint16_t v2r = UPLOAD(voice_2_right_vag);

    for (;;) {
        play(v1l, 0, 0x3fff, 0);
        delay(120);
        
        play(v1r, 0, 0, 0x3fff);
        delay(120);

        play(v2l, 1, 0x3fff, 0);
        delay(120);

        play(v2r, 1, 0, 0x3fff);
        delay(120);

        play(v1l, 0, 0x3fff, 0);
        play(v2r, 1, 0, 0x3fff);
        
        delay(120);
    }
    return 0;
}
