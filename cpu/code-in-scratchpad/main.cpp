#include <common.h>
#include <psxapi.h>
#include <string.h>
#include <exception.hpp>
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
    hookUnresolvedExceptionHandler(exceptionHandler);
    ExitCriticalSection();

    memcpy((void*)RAM_FUN_ADDR, (void*)codeRam, 64);
    memcpy((void*)SCRATCHPAD_FUN_ADDR, (void*)codeScratchpad, 64);

    FlushCache();

    wasExceptionThrown();
    
    func codeInRam = (func)RAM_FUN_ADDR;
    func codeInScratchpad = (func)SCRATCHPAD_FUN_ADDR;

    printf("Executing code from RAM: ");
    codeInRam();
    if (!wasExceptionThrown()) printf("ok.\n");
    else printf("fail (unexpected exception).\n");

    printf("Executing code from Scratchpad: ");
    codeInScratchpad();
    if (wasExceptionThrown()) printf("ok.\n");
    else printf("fail (no bus error exception thrown).\n");
    
    printf("Done\n");


    for (;;) {
        VSync(0);
    }
    return 0;
}
