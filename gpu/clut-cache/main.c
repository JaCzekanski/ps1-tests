#include <common.h>

enum TextureMode { bit4 = 0, bit8 = 1, bit15 = 2, reserved = 3 };
void setE1(enum TextureMode mode, int texPageX, int texPageY) {
    DR_TPAGE e;
    unsigned short texpage = getTPage((int)mode, /*transparencyMode*/ 0, texPageX, texPageY);
    setDrawTPage(&e, /* Drawing to display area */ 1, /*dithering*/ 0, texpage);
    DrawPrim(&e);
}

void rectangle(int x, int y, int u, int v, int clutx, int cluty) {
    SPRT s;
    setSprt(&s);
    setSemiTrans(&s, false);
    setXY0(&s, x, y);
    setWH(&s, 256, 1);
    setRGB0(&s, 0x80, 0x80, 0x80);
    setClut(&s, clutx, cluty);
    setUV0(&s, u, v);
   
    DrawPrim(&s);
}

void uploadToGPU(uint16_t* buffer, uint16_t x, uint16_t y, int words) {
    DrawSync(0);
    CPU2VRAM buf = {0};
    setcode(&buf, 0xA0); // CPU -> VRAM
    setlen(&buf, 3);
    setXY0(&buf, x, y);
    setWH(&buf, words, 1);
    DrawPrim(&buf);

    volatile uint32_t* GP0 = (uint32_t*)0x1F801810;
	for (int n = 0; n < words; n += 2) {
        uint16_t pixel1 = buffer[n];
        uint16_t pixel2 = buffer[n+1];
		*GP0 = pixel1 | (pixel2<<16);
	}
}

void line(int sx, int sy, int ex, int ey, int r, int g, int b) {
    LINE_F2 l;
    setLineF2(&l);
    setRGB0(&l, r, g, b);

    l.x0 = sx;
    l.y0 = sy;
    l.x1 = ex;
    l.y1 = ey;

    DrawPrim(&l);
}

void writeTestClut(int x, int y) {
    uint16_t buffer[256];
	for (int n = 0; n < 256; n++) { 
        buffer[n] = n;
	}
    uploadToGPU(buffer, x, y, 256);
}

void gpuClearCache() {
    volatile uint32_t* GP0 = (uint32_t*)0x1F801810;
    *GP0 = 0x01000000;
}

void writeTextureLinear(int x, int y) {
    uint16_t buffer[256];
    for (int i = 0, ptr = 0; i<256; i+=2) {
        uint16_t p1 = i;
        uint16_t p2 = (i+1);
        buffer[ptr++] = p1 | (p2 << 8);
    }
    uploadToGPU(buffer, x, y, 128);
}

void writeTextureLinearReversed(int x, int y) {
    uint16_t buffer[256];
    for (int i = 0, ptr = 0; i<256; i+=2) {
        uint16_t p1 = 255 - i;
        uint16_t p2 = 255 - (i+1);
        buffer[ptr++] = p1 | (p2 << 8);
    }
    uploadToGPU(buffer, x, y, 128);
}

void writeTextureRandom(int x, int y) {
    uint16_t buffer[256];
    for (int i = 0, ptr = 0; i<256; i+=2) {
        uint16_t p1 = i * (i<<3);
        uint16_t p2 = (i+1) * ((i+1)<<3);
        buffer[ptr++] = p1 | (p2 << 8);
    }
    uploadToGPU(buffer, x, y, 128);
}

int y = 20;

void testDummy() {
    writeTestClut(0, y);
    gpuClearCache();

    y += 16;
}

void testLinearTexture() {
    writeTextureLinear(0, 1);
    writeTestClut(0, y);
    gpuClearCache();
    rectangle(0, y, 0, 1, 0, y);

    y += 16;
}

void testReverseLinearTexture() {
    writeTextureLinearReversed(0, 2);
    writeTestClut(0, y);
    gpuClearCache();
    rectangle(0, y, 0, 2, 0, y);

    y += 16;
}

void testRandomTexture() {
    writeTextureRandom(0, 3);
    writeTestClut(0, y);
    gpuClearCache();
    rectangle(0, y, 0, 3, 0, y);

    y += 16;
}

void testClutCacheReuseNoClear() {
    writeTestClut(0, y);
    
    // Write textured rectangle
    rectangle(0, y+2, 0, 1, 0, y);

    // Overwrite CLUT in VRAM without telling GPU about it
    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);

    // Write textured rectangle again (cached CLUT should be used)
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

void testClutCacheReuseClear() {
    writeTestClut(0, y);
  
    rectangle(0, y+2, 0, 1, 0, y);

    // Overwrite CLUT in VRAM, but issue the clear cache command
    line(0, y, 256, y, 0xff, 0xff, 0xff);
    gpuClearCache();

    // Write textured rectangle again (cached CLUT should be used)
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

void testClutCacheInvalidatedDifferentClut() {
    writeTestClut(0, y);
  
    rectangle(0, y+2, 0, 1, 0, y);

    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);
   
    // Write textured rectangle again (CLUT$ should be invalidated due to different clutx used)
    rectangle(0, y+4, 0, 1, 16, y);

    y += 16;
}

void testClutCacheNotInvalidatedAfterChangingTextureModeFrom4to8() {
    writeTestClut(0, y);

    // GPU should load only 16 entries into CLUT since 4bit textures are used
    setE1(bit4, 0, 0);
    rectangle(0, y+2, 0, 1, 0, y);

    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);

    // GPU should reload the CLUT into cache since additional entries are needed
    setE1(bit8, 0, 0);
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

void testClutCacheInvalidatedAfterChangingTextureModeFrom8to4() {
    writeTestClut(0, y);

    // GPU should load all 256 entries into cache
    setE1(bit8, 0, 0);
    rectangle(0, y+2, 0, 1, 0, y);

    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);

    // GPU shouldn't reload CLUT since all needed entries are already there
    setE1(bit4, 0, 0);
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

void testClutCacheInvalidatedAfterChangingTextureModeFrom15to4() {
    writeTestClut(0, y);

    // GPU shouldn't load anything into CLUT$
    setE1(bit15, 0, 0);
    rectangle(0, y+2, 0, 1, 0, y);

    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);

    // GPU should reload CLUT since all nothing was there
    setE1(bit4, 0, 0);
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

// Same as in  testClutCacheInvalidatedAfterChangingTextureModeFrom15to4, but with "reserved" bit depth (same as 15bit) to 8bit
void testClutCacheInvalidatedAfterChangingTextureModeFromReservedTo8() {
    writeTestClut(0, y);

    setE1(reserved, 0, 0);
    rectangle(0, y+2, 0, 1, 0, y);

    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);

    setE1(bit8, 0, 0);
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

void testE1NotInvalidatingCache() {
    writeTestClut(0, y);

    // Load only 16 entries into cache
    setE1(bit4, 0, 0);
    rectangle(0, y+2, 0, 1, 0, y);

    fillRect(0, y, 256, 1, 0xff, 0xff, 0xff);

    setE1(bit8, 0, 0); // Setting different texture mode shouldn't cause invalidation
    setE1(bit4, 0, 0); // until something is rendered
    // GPU shouldn't reload CLUT since all needed entries are already there
    rectangle(0, y+4, 0, 1, 0, y);

    y += 16;
}

int main() {
    initVideo(320, 240);
    printf("\ngpu/clut-cache\n");
    printf("GPU caches the palette/CLUT before rendering 4/8bit textured primitives.\n");
    printf("This test check this by rendering textured rectangle over currently used CLUT (first 4 lines).\n");
    printf("Next 3 tests check if Clear Cache or using different CLUT position does invalidate the CLUT$.\n");
    printf("Last 5 tests verify when CLUT$ is reloaded when different texture modes are used.\n");
    printf("No assertions are made, please compare output with the VRAM dump.\n\n");

    clearScreen();
	DrawSync(0);
    setE1(bit8, 0, 0);
    
    // Checks is CLUT$ implemented
    testDummy();
    testLinearTexture();
    testReverseLinearTexture();
    testRandomTexture();
    
    // Checks when CLUT$ is invalidated (CLUT position change)
    testClutCacheReuseNoClear();
    testClutCacheReuseClear();
    testClutCacheInvalidatedDifferentClut();
    
    // Checks when CLUT$ is invalidated (texture mode change)
    testClutCacheNotInvalidatedAfterChangingTextureModeFrom4to8();    
    testClutCacheInvalidatedAfterChangingTextureModeFrom8to4();
    testClutCacheInvalidatedAfterChangingTextureModeFrom15to4();
    testClutCacheInvalidatedAfterChangingTextureModeFromReservedTo8();
    testE1NotInvalidatingCache();

    DrawSync(0);
    printf("Done\n");
    
    for (;;) {
        VSync(0);
    }
    return 0;
}
