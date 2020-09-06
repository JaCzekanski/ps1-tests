#include <common.h>
#include <psxapi.h>
#include <string.h>
#include <exception.hpp>
#include <test.h>
#include <io.h>
#include "code.h"

typedef uint32_t (*func)();

void memcpy32(uint32_t* dst, uint32_t* src, size_t words) {
    for (size_t i = 0; i<words; i++) {
        *dst++ = *src++;
    }
}

__attribute__((noinline)) func copyCodeTo(uint32_t addr) {
    EnterCriticalSection();

    memcpy32((uint32_t*)addr, (uint32_t*)testCode, 2);
    
    FlushCache();
    ExitCriticalSection();

    return (func)addr;
}

__attribute__((noinline)) void testCodeInRam() {
    func tested = copyCodeTo(0x00040000);
    tested();

    assertFalse(wasExceptionThrown());
}

__attribute__((noinline)) void testCodeInScratchpad() {
    func tested = copyCodeTo(0x1F800000);
    tested();

    TEST_MULTIPLE_BEGIN();
    assertTrue(wasExceptionThrown());
    assertEquals(getExceptionType(), cop0::CAUSE::Exception::busErrorInstruction);
    TEST_MULTIPLE_END();
}

__attribute__((noinline)) void testCodeInMDEC() {
    func tested = copyCodeTo(0x1F801820);
    tested();

    TEST_MULTIPLE_BEGIN();
    assertTrue(wasExceptionThrown());
    assertEquals(getExceptionType(), cop0::CAUSE::Exception::busErrorInstruction);
    TEST_MULTIPLE_END();
}

__attribute__((noinline)) void testCodeInInterrupts() {
    EnterCriticalSection();
    uint32_t imask = read32(0x1F801074);
    
    func tested = copyCodeTo(0x1F801070);
    tested();

    TEST_MULTIPLE_BEGIN();
    assertTrue(wasExceptionThrown());
    assertEquals(getExceptionType(), cop0::CAUSE::Exception::busErrorInstruction);
    TEST_MULTIPLE_END();

    write32(0x1F801074, imask);
    ExitCriticalSection();
}

__attribute__((noinline)) void testCodeInSPU() {
    func tested = copyCodeTo(0x1F801C00);
    tested();

    assertFalse(wasExceptionThrown());
}

__attribute__((noinline)) void testCodeInDMA0() {
    func tested = copyCodeTo(0x1F8010F0);
    tested();

    assertFalse(wasExceptionThrown());
}

__attribute__((noinline)) void testCodeInDMAControl() {
    func tested = copyCodeTo(0x1F801084);
    tested();

    assertFalse(wasExceptionThrown());
}

__attribute__((noinline)) void runTests() {
    testCodeInRam();
    testCodeInScratchpad();
    testCodeInMDEC();
    testCodeInInterrupts();
    testCodeInSPU();
    testCodeInDMA0();
    testCodeInDMAControl();

    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\ncpu/code-in-io\n");
    printf("Test code execution from Scratchpad and various IO ports\n");

    clearScreen();
    
    EnterCriticalSection();
    hookUnresolvedExceptionHandler(exceptionHandler);
    ExitCriticalSection();

    wasExceptionThrown(); // Clear flag
    runTests();

    for (;;) {
        VSync(0);
    }
    return 0;
}
