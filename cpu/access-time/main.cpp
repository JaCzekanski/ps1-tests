#include <common.h>
#include <measure.hpp>

static void doNotOptimize(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

template <typename T, uint32_t base_address, int count = 4000>
void readAccessTimeBitSize() {
/*
800100c4:	24020064 	li	v0,100
        asm("nop");
800100c8:	00000000 	nop
800100cc:	2442ffff 	addiu	v0,v0,-1
        for (int i = 0; i<count; i++) {
800100d0:	1440fffd 	bnez	v0,800100c8 <void readAccessTime<2147483648u, 100>(char const*)+0x50>
800100d4:	00000000 	nop
*/
    uint32_t measuredCyclesNop = measure<MeasureMethod::CpuClock>([] {
        volatile T* ptr = (T*)(base_address);
        for (int i = 0; i<count; i++) {
            asm("nop");
        }
        doNotOptimize((void*)ptr);
    });
/*
8001012c:	3c038000 	lui	v1,0x8000
            asm("nop");
80010130:	00000000 	nop
80010134:	2442ffff 	addiu	v0,v0,-1
            *ptr;
80010138:	90640000 	lbu	a0,0(v1)
        for (int i = 0; i<count; i++) {
8001013c:	1440fffc 	bnez	v0,80010130 <void readAccessTime<2147483648u, 100>(char const*)+0xb8>
80010140:	00000000 	nop
*/
    uint32_t measuredCycles = measure<MeasureMethod::CpuClock>([] {
        volatile T* ptr = (T*)(base_address);
        for (int i = 0; i<count; i++) {
            asm("nop");
            *ptr;
        }
        doNotOptimize((void*)ptr);
    });

    measuredCycles -= measuredCyclesNop;

    printf("%2d.%2-d    ", measuredCycles/count, measuredCycles%count);
}

template <uint32_t base_address, int count = 100>
void readAccessTime(const char* memoryType) {
    printf("%-10s (0x%08x)   ", memoryType, base_address);
    readAccessTimeBitSize<uint8_t,  base_address, count>();
    readAccessTimeBitSize<uint16_t, base_address, count>();
    readAccessTimeBitSize<uint32_t, base_address, count>();
    printf("\n");
}

void runTests() {
    printf("SEGMENT    (   ADDRESS)    8bit    16bit    32bit    (cpu cycles per bitsize)\n");
    readAccessTime<0x80000000>("RAM");
    readAccessTime<0xBFC00000>("BIOS");
    readAccessTime<0x1F800000>("SCRATCHPAD");
    readAccessTime<0x1F000000>("EXPANSION1");
    readAccessTime<0x1F802000>("EXPANSION2");
    readAccessTime<0x1FA00000>("EXPANSION3");
    readAccessTime<0x1F8010F0>("DMAC_CTRL");
    readAccessTime<0x1F801044>("JOY_STAT");
    readAccessTime<0x1F801054>("SIO_STAT");
    readAccessTime<0x1F801060>("RAM_SIZE");
    readAccessTime<0x1F801070>("I_STAT");
    readAccessTime<0x1F801100>("TIMER0_VAL");
    readAccessTime<0x1F801800>("CDROM_STAT");
    readAccessTime<0x1F801814>("GPUSTAT");
    readAccessTime<0x1F801824>("MDECSTAT");
    readAccessTime<0x1F801DAA>("SPUCNT");
    readAccessTime<0xFFFE0130>("CACHECTRL");

    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    clearScreen();
    printf("\ncpu/access-time\n");
    printf("Test CPU access time to different parts of memory map.\n");
    printf("There are no assertions - please manually compare your results with provided psx.log.\n\n");

    runTests();

    for (;;) {
        VSync(false);
    }

    return 0;
}
