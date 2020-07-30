#pragma once
#include <stdint.h>
#include "cop0.h"

typedef void (*void_fn_ptr)();

struct Registers {
    uint32_t r[32];
    uint32_t returnPC;
    uint32_t hi, lo;
    cop0::STATUS sr;
    cop0::CAUSE cause;
};

struct Thread {
    uint32_t flags, flags2;
    Registers registers;
    uint32_t unknown[9];
};

struct Process {
    Thread* thread;
};

Thread* getCurrentThread();
void hookUnresolvedExceptionHandler(void_fn_ptr fn);
bool wasExceptionThrown();
cop0::CAUSE::Exception getExceptionType();
void exceptionHandler();
