#include <common.h>
#include <test.h>

const uint32_t E1_MASK = 0b10000111'11111111;

void drawTexturedPolygon(uint16_t texpage) {
    POLY_FT4 p = {0};
    setPolyFT4(&p);
    setRGB0(&p, 0x80, 0x80, 0x80);
    setClut(&p, 0, 0);
    setUVWH(&p, 0, 0,255, 255);
    setXY4(&p, 
        0, 0,
        0, 32,
        32, 0,
        32, 32
    );
    p.tpage = texpage; 
    
    DrawPrim(&p);
}

// Check if writing zeroes to GP0_E1 == what GPSTAT read
void testWriteZerosToE1() {
    writeGP1(0x09, 0); // Disable E1.11 bit
    writeGP0(0xe1,0x000);
    
    assertEquals((ReadGPUstat() & E1_MASK), 0b00000000'00000000);
}

// Check if writing ones to GP0_E1 == what GPSTAT read (no "Texture Disable")
void testWriteOnesToE1() {
    writeGP1(0x09, 0); // Disable E1.11 bit
    writeGP0(0xe1, 0xfff);

    assertEquals((ReadGPUstat() & E1_MASK), 0b00000111'11111111);
}

// Check if writing ones to GP0_E1 == what GPSTAT read (with "Texture Disable")
void testWriteOnesToE1WithTextureDisable() {
    writeGP1(0x09, 1); // Enable E1.11 bit
    writeGP0(0xe1,0xfff);

    assertEquals((ReadGPUstat() & E1_MASK), 0b10000111'11111111);
}

// Check which bits in GP0_E1 are changed by Textured Polygon draw commands
//        vv - These two bits can be set via GP0_E1, but not via draw command
// 0b10000111'11111111
void testTexturedPolygons() {
    writeGP1(0x09, 0); // Disable E1.11 bit
    writeGP0(0xe1,0x000);
    drawTexturedPolygon(0xffff); 

    assertEquals((ReadGPUstat() & E1_MASK), 0b0000001'11111111);
}

// Check which bits in GP0_E1 are changed by Textured Polygon draw commands (Texture Disable allowed)
void testTexturedPolygonsTextureDisable() {
    writeGP1(0x09, 1); // Enable E1.11 bit
    writeGP0(0xe1,0x000);
    drawTexturedPolygon(0xffff); 

    assertEquals((ReadGPUstat() & E1_MASK), 0b10000001'11111111);
}

// Check if drawing textured polygons doesn't change other bits in E1 register
void testTexturedPolygonsDoesNotChangeOtherBits() {
    writeGP1(0x09, 1); // Enable E1.11 bit
    writeGP0(0xe1,0xfff);
    drawTexturedPolygon(0x0000); 

    assertEquals((ReadGPUstat() & E1_MASK), 0b00000110'00000000);
}

void runTests() {
    testWriteZerosToE1();
    testWriteOnesToE1();
    testWriteOnesToE1WithTextureDisable();
    testTexturedPolygons();
    testTexturedPolygonsTextureDisable();
    testTexturedPolygonsDoesNotChangeOtherBits();
    
    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\ngpu/gp0-e1\n");

    clearScreen();
    writeGP0(1, 0); // Wait for gpu to complete all commands

    runTests();
    
    for (;;) {
        VSync(0);
    }
    return 0;
}
