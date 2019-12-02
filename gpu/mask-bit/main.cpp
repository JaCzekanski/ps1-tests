#include <common.h>
#include <test.h>

// Check if read pixel == written pixel
void testWriteAndRead() {
    int x = 32;
    int y = 32;
    
    setMaskBitSetting(false, false);
    
    vramPut(x, y, 0x1234);

    assertEquals(vramGet(x, y), 0x1234);
}

// Check if GP0(0xE6) bit0 (Set mask while drawing) works
void testSetBit() {
    int x = 33;
    int y = 32;
    
    setMaskBitSetting(true, false);
    vramPut(x, y, 0);

    assertEquals(vramGet(x, y), 0x8000);
}

// Check if GP0(0xE6) bit1 (Check mask before draw) works
void testCheckMaskBit() {
    int x = 34;
    int y = 32;
    
    // Disable mask bit set
    setMaskBitSetting(false, false);
    vramPut(x, y, 0x8000); // Write mask bit

    // Enable check mask bit
    setMaskBitSetting(false, true);
    vramPut(x, y, 0x1234);

    assertEquals(vramGet(x, y), 0x8000);
}

// Check mask bit (written manually) can be overwritten
void testCheckIsMaskBitStickyManually() {
    int x = 35;
    int y = 32;
    
    setMaskBitSetting(false, false);
    vramPut(x, y, 0x8123); // Write mask bit manually
    vramPut(x, y, 0x0456); // Try clearing it

    assertEquals(vramGet(x, y), 0x0456);
}

// Check mask bit (written automatically) can be overwritten
void testCheckIsMaskBitStickySetBit() {
    int x = 36;
    int y = 32;
    
    setMaskBitSetting(true, false);
    vramPut(x, y, 0x0000); // Write mask bit using HW

    setMaskBitSetting(false, false);
    vramPut(x, y, 0x0456); // Try clearing it

    assertEquals(vramGet(x, y), 0x0456);
}

void runTests() {
    testWriteAndRead();
    testSetBit();
    testCheckMaskBit();
    testCheckIsMaskBitStickyManually();
    testCheckIsMaskBitStickySetBit();
    
    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\ngpu/mask-bit\n");

    setMaskBitSetting(false, false);
    clearScreen();

    runTests();
    
    for (;;) {
        VSync(0);
    }
    return 0;
}
