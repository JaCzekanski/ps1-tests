#include <common.h>
#include <test.h>

const uint32_t E1_MASK = 0b10000111'11111111;

void drawTexturedPolygon(uint16_t texpage) {
    POLY_FT4 p;
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

// Allow setting E1.11 bit
void allowTextureDisable(bool allow) {
    writeGP1(0x09, allow);
}

// Check if writing zeroes to GP0_E1 == what GPSTAT read
void testWriteZerosToE1() {
    allowTextureDisable(false); // Disable E1.11 bit
    writeGP0(0xe1, 0x000);
    
    assertEquals((ReadGPUstat() & E1_MASK), 0b00000000'00000000);
}

// Check if writing ones to GP0_E1 == what GPSTAT read (no "Texture Disable")
void testWriteOnesToE1() {
    allowTextureDisable(false);
    writeGP0(0xe1, 0xfff);

    assertEquals((ReadGPUstat() & E1_MASK), 0b00000111'11111111);
}

// Check if writing ones to GP0_E1 == what GPSTAT read (with "Texture Disable")
void testWriteOnesToE1WithTextureDisable() {
    allowTextureDisable(true);
    writeGP0(0xe1, 0xfff);

    assertEquals((ReadGPUstat() & E1_MASK), 0b10000111'11111111);
}

// Check which bits in GP0_E1 are changed by Textured Polygon draw commands
//        vv - These two bits can be set via GP0_E1, but not via draw command
// 0b10000111'11111111
void testTexturedPolygons() {
    allowTextureDisable(false);
    writeGP0(0xe1, 0x000);
    drawTexturedPolygon(0xffff); 

    assertEquals((ReadGPUstat() & E1_MASK), 0b0000001'11111111);
}

// Check which bits in GP0_E1 are changed by Textured Polygon draw commands (Texture Disable allowed)
void testTexturedPolygonsTextureDisable() {
    allowTextureDisable(true);
    writeGP0(0xe1, 0x000);
    drawTexturedPolygon(0xffff); 

    assertEquals((ReadGPUstat() & E1_MASK), 0b10000001'11111111);
}

// Check if drawing textured polygons doesn't change other bits in E1 register
void testTexturedPolygonsDoesNotChangeOtherBits() {
    allowTextureDisable(true);
    writeGP0(0xe1,0xfff);
    drawTexturedPolygon(0x0000); 

    assertEquals((ReadGPUstat() & E1_MASK), 0b00000110'00000000);
}

// Check if write to "Texture disabled" bit in e1 is ignored when "Allow Texture Disable" is unset
void testTextureDisableBitIsIgnoredWhenNotAllowed() {
    allowTextureDisable(false);
    writeGP0(0xe1, 1<<11); // Write Texture disabled bit

    assertEquals(ReadGPUstat() & 0b10000000'00000000, 0 << 15);
}

// Check if write to "Texture disabled" bit in e1 is allowed when "Allow Texture Disable" is set
void testTextureDisableBitIsWrittenWhenAllowed() {
    allowTextureDisable(true);
    writeGP0(0xe1, 1<<11);

    assertEquals(ReadGPUstat() & 0b10000000'00000000, 1 << 15);
}

// Check if "Texture disabled" bit is preserved when disabling "Allow Texture Disable"
// Conclusion - "Allow Texture Disable" Bit only prevents write to this bit, read is unaffected
void testUnsetAllowTextureDisablePreservesBit() {
    allowTextureDisable(true);
    writeGP0(0xe1, 1<<11);
    allowTextureDisable(false);

    assertEquals(ReadGPUstat() & 0b10000000'00000000, 1 << 15);
}

// Check if "Texture disabled" bit is cleared after write if "Allow Texture Disable" is unset
// Note - you cannot preserve this bit in e1 after write 
void testUnsetAllowTextureDisableClearsBitAfterWrite() {
    allowTextureDisable(true);
    writeGP0(0xe1, 1<<11);
    allowTextureDisable(false); 
    writeGP0(0xe1, 0); // Clear Texture disabled bit

    assertEquals(ReadGPUstat() & 0b10000000'00000000, 0);
}

void runTests() {
    testWriteZerosToE1();
    testWriteOnesToE1();
    testWriteOnesToE1WithTextureDisable();
    testTexturedPolygons();
    testTexturedPolygonsTextureDisable();
    testTexturedPolygonsDoesNotChangeOtherBits();
    testTextureDisableBitIsIgnoredWhenNotAllowed();
    testTextureDisableBitIsWrittenWhenAllowed();
    testUnsetAllowTextureDisablePreservesBit();
    testUnsetAllowTextureDisableClearsBitAfterWrite();
    
    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\ngpu/gp0-e1\n");

    clearScreen();
    writeGP0(1, 0); // Wait for gpu to complete all commands

    runTests();
    
    for (;;) {
        VSync(false);
    }
    return 0;
}
