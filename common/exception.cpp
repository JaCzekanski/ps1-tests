#include <psxapi.h>
#include "exception.hpp"

// #define DEBUG

// From __globals table
static uint32_t* A0 = (uint32_t*)0x200;

void hookUnresolvedExceptionHandler(void_fn_ptr fn) {
    A0[0x40] = (uint32_t)fn;
}

Thread* getCurrentThread() {
    Process** processes = (struct Process**)0x108;
    return processes[0]->thread;
}

bool wasExceptionThrown() {
    Thread* currentThread = getCurrentThread();

    bool ret = getCurrentThread()->unknown[0];
    currentThread->unknown[0] = 0;
    return ret;
}

cop0::CAUSE::Exception getExceptionType() {
    Thread* currentThread = getCurrentThread();

    return (cop0::CAUSE::Exception)getCurrentThread()->unknown[1];
}

void exceptionHandler() {
    Thread* currentThread = getCurrentThread();
    auto exception = currentThread->registers.cause.exception;

    currentThread->unknown[0] = 1; // Exception was thrown flag
    currentThread->unknown[1] = (int)exception;

#ifdef DEBUG
    printf("\n"
        "!! Custom exception handler\n"
        "!! Exception '%s' (%d) thrown at address 0x%08x\n", exceptionName[(int)exception], (int)exception, currentThread->registers.returnPC);
#endif

    currentThread->registers.returnPC += 4; // Skip failing instruction
}