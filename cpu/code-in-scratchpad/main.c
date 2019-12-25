#include <common.h>
#include <psxapi.h>
#include <string.h>
#include "code.h"

uint32_t RAM_FUN_ADDR = 0x00040000;
uint32_t SCRATCHPAD_FUN_ADDR = 0x1F800000;

typedef uint32_t (*func)();

// TODO: This test is not fully finished
// It is only for testing whether PS1 can execute code from Scratchpad

int main() {
    initVideo(320, 240);
    printf("\ncpu/code-in-scratchpad\n");

    clearScreen();
    
    EnterCriticalSection();

    memcpy((void*)RAM_FUN_ADDR, codeRam, 64);
    memcpy((void*)SCRATCHPAD_FUN_ADDR, codeScratchpad, 64);

    FlushCache();
    
    func codeInRam = (func)RAM_FUN_ADDR;
    func codeInScratchpad = (func)SCRATCHPAD_FUN_ADDR;

    printf("Code in ram returned PC: 0x%08x\n", codeInRam());
    printf("Code in scratchpad returned PC: 0x%08x\n", codeInScratchpad());
    // ^^^ This line crashes on real HW

    printf("Done\n");

    ExitCriticalSection();
    for (;;) {
        VSync(0);
    }
    return 0;
}
