#include <common.h>
#include <psxapi.h>
#include <io.h>
#include <exception.hpp>
#include "asm.h"

template <typename T, uint32_t address>
void writeTo(const char* memoryType) {
    auto logValue = [](uint32_t value){
        if (wasExceptionThrown()) {
            printf("%-10s    ", "--CRASH--");
        } else {
            printf("%#10x    ", value);
        }
    };

    printf("%-10s (0x%08x)   ", memoryType, address);

    uint32_t value = 0x12345678;

    T preservedValue = *((volatile T*)address);
    
    if constexpr (sizeof(T) >= 1) {
        *((volatile T*)address) = 0;
        asm_write_8(address, value);
        uint32_t result8 = *((volatile T*)address);
        logValue(result8);
    }

    if constexpr (sizeof(T) >= 2) {                
        *((volatile T*)address) = 0;
        asm_write_16(address, value);
        uint32_t result16 = *((volatile T*)address);
        logValue(result16);
    }

    if constexpr (sizeof(T) >= 4) {
        *((volatile T*)address) = 0;
        asm_write_32(address, value);
        uint32_t result32 = *((volatile T*)address);
        logValue(result32);
    }

    *((volatile T*)address) = preservedValue;
    printf("\n");
}

template <typename T>
void runTests() {
    printf("\nReading as %dbit, writing as:\n", sizeof(T)*8);
    printf("SEGMENT    (   ADDRESS)   ");
    if constexpr (sizeof(T) >= 1) printf("8bit          ");
    if constexpr (sizeof(T) >= 2) printf("16bit         ");
    if constexpr (sizeof(T) >= 4) printf("32bit         ");
    printf("\n");

    write32(0x1f801020, 0x00001325); // COM_DELAY

    writeTo<T, 0x80080000>("RAM");
    writeTo<T, 0xBFC00000>("BIOS"); // RO
    writeTo<T, 0x1F800000>("SCRATCHPAD");
    
    // write32(0x1F801000, 0x1F000000); // Exp1 base
    // write32(0x1F801008, 0x0013243F); // Exp1 Delay/Size
    // writeTo<T, 0x1F000000>("EXPANSION1"); // Stalls the console ¯\_(ツ)_/¯
  
    write32(0x1F801004, 0x1F802000); // Exp2 base
    write32(0x1F80101C, 0x00070777); // Exp2 Delay/Size
    writeTo<T, 0x1F802000>("EXPANSION2");

    write32(0x1F80100C, 0x00003022); // Exp3 Delay/Size
    writeTo<T, 0x1FA00000>("EXPANSION3");

    writeTo<T, 0x1F801080>("DMA0_ADDR");
    writeTo<T, 0x1F8010F0>("DMAC_CTRL");
    writeTo<T, 0x1F8010F4>("DMAC_INTR");

    writeTo<T, 0x1F801048>("JOY_MODE"); // Crash on 32bit
    writeTo<T, 0x1F80104A>("JOY_CTRL");
    writeTo<T, 0x1F801058>("SIO_MODE"); // Crash on 32bit
    writeTo<T, 0x1F80105A>("SIO_CTRL");
    
    // writeTo<T, 0x1F801060>("RAM_SIZE"); // Stalls the console
    writeTo<T, 0x1F801074>("I_MASK");
    writeTo<T, 0x1F801108>("T0_TARGET");

    write32(0x1f801018, 0x00020943); // CDROM Delay/Size
    writeTo<T, 0x1F801800>("CDROM_STAT");
    writeTo<T, 0x1F801814>("GPUSTAT");
    writeTo<T, 0x1F801824>("MDECSTAT");
    
    write32(0x1F801014, 0x220931E1); // SPU Delay/Size
    writeTo<T, 0x1F801DAA>("SPUCNT"); // Crash on 32bit

    // writeTo<T, 0xFFFE0130>("CACHECTRL"); // Stalls the console
}

int main() {
    initVideo(320, 240);
    clearScreen();
    printf("\ncpu/io-access-bitwidth\n");
    printf("Test how writes with different bitwidths behaves for different io devices\n");
    printf("Note: Disable \"Exception Handling Surveillance\" in Caetla first\"\n");

    EnterCriticalSection();
    hookUnresolvedExceptionHandler(exceptionHandler);
    wasExceptionThrown(); // Clear flag
   
    runTests<uint32_t>();
    runTests<uint16_t>();
    runTests<uint8_t>();
    printf("Done.\n");
   
    ExitCriticalSection();

    for (;;) {
        VSync(false);
    }

    return 0;
}
