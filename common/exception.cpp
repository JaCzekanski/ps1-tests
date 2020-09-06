#include <psxapi.h>
#include "exception.hpp"
// #define DEBUG

#ifdef DEBUG
#include <stdio.h>

const char* exceptionName[] = {
    "interrupt", //  0
    "exception1", // 1
    "exception2", // 2
    "exception3", // 3
    "addressErrorLoad", // 4
    "addressErrorStore", // 5
    "busErrorInstruction", // 6
    "busErrorData", // 7
    "syscall", // 8
    "breakpoint", // 9
    "reservedInstruction", // 10
    "coprocessorUnusable", // 11
    "arithmeticOverflow", // 12
};
#endif

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

    bool ret = currentThread->unknown[0];
    currentThread->unknown[0] = 0;
    return ret;
}

cop0::CAUSE::Exception getExceptionType() {
    Thread* currentThread = getCurrentThread();

    return (cop0::CAUSE::Exception)currentThread->unknown[1];
}

void exceptionHandler() {
    Thread* currentThread = getCurrentThread();
    auto exception = currentThread->registers.cause.exception;

    currentThread->unknown[0] = 1; // Exception was thrown flag
    currentThread->unknown[1] = (int)exception;

#ifdef DEBUG
    printf("\n"
        "!! Custom exception handler\n"
        "!! Exception '%s' (%d) thrown at address 0x%08x\n"
        "!! r31 (ra): 0x%08x\n"
        "!! COP0 STATUS: 0x%08x\n"
        "!! COP0 CAUSE: 0x%08x\n", 
        exceptionName[(int)exception], 
        (int)exception, 
        currentThread->registers.returnPC, 
        currentThread->registers.r[31],
        currentThread->registers.sr._reg,
        currentThread->registers.cause._reg
    );
#endif

    if (exception == cop0::CAUSE::Exception::busErrorInstruction) {
        currentThread->registers.returnPC = currentThread->registers.r[31]; // return to RA
    } else {
        currentThread->registers.returnPC += 4; // Skip failing instruction
    }
}