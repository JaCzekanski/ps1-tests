#include <common.h>
#include <psxapi.h>
#include <string.h>
#include <exception.hpp>
#include <test.h>
#include <io.h>
#include "code.h"

typedef uint32_t (*func)();

__attribute__((noinline)) func copyCodeTo(uint32_t addr) {
    EnterCriticalSection();
    memcpy((void*)addr, (void*)testCode, 8);
    
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

    assertTrue(wasExceptionThrown());
}

__attribute__((noinline)) void testCodeInMDEC() {
    func tested = copyCodeTo(0x1F801820);
    tested();

    assertTrue(wasExceptionThrown());
}

__attribute__((noinline)) void testCodeInInterrupts() {
    EnterCriticalSection();
    uint32_t imask = read32(0x1F801074);
    ExitCriticalSection();
    
    func tested = copyCodeTo(0x1F801070);
    tested();

    assertTrue(wasExceptionThrown());

    EnterCriticalSection();
    write32(0x1F801074, imask);
    ExitCriticalSection();
}

__attribute__((noinline)) void runTests() {
    testCodeInRam();
    testCodeInScratchpad();
    testCodeInMDEC();
    testCodeInInterrupts();

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
