#pragma once
#include <stdint.h>

// From Avocado (src/cpu/cop0.h)

namespace cop0 {
    // cop0r7 DCIC - breakpoint control
    union DCIC {
        struct {
            uint32_t breakpointHit : 1;
            uint32_t codeBreakpointHit : 1;
            uint32_t dataBreakpointHit : 1;
            uint32_t dataReadBreakpointHit : 1;
            uint32_t dataWriteBreakpointHit : 1;
            uint32_t jumpBreakpointHit : 1;
            uint32_t : 6;  // not used

            uint32_t jumpRedirection : 2;  // 0 - disabled, 1..3 - enabled

            uint32_t : 2;  // Unknown
            uint32_t : 7;  // not used

            uint32_t superMasterEnable1 : 1;  // bits 24..29

            uint32_t breakOnCode : 1;
            uint32_t breakOnData : 1;
            uint32_t breakOnDataRead : 1;
            uint32_t breakOnDataWrite : 1;
            uint32_t breakOnJump : 1;

            uint32_t masterEnableBreakAnyJump : 1;
            uint32_t masterEnableBreakpoints : 1;  // bits 24..27
            uint32_t superMasterEnable2 : 1;       // bits 24..29
        };

        uint32_t _reg;
    };
    // cop0r13 cause, ro, bit8-9 are rw
    union CAUSE {
        enum class Exception {
            interrupt = 0,
            addressErrorLoad = 4,
            addressErrorStore = 5,
            busErrorInstruction = 6,
            busErrorData = 7,
            syscall = 8,
            breakpoint = 9,
            reservedInstruction = 10,
            coprocessorUnusable = 11,
            arithmeticOverflow = 12
        };

        struct {
            uint32_t : 2;
            Exception exception : 5;
            uint32_t : 1;
            uint32_t interruptPending : 8;
            uint32_t : 12;
            uint32_t coprocessorNumber : 2;  // If coprocessor caused the exception
            uint32_t branchTaken : 1;        /** When the branchDelay bit is set, the branchTaken Bit determines whether or not the
                                              * branch is taken. A value of one in branchTaken indicates that the branch is
                                              * taken. The Target Address Register holds the return address.
                                              *
                                              * source: L64360 datasheet
                                              */
            uint32_t branchDelay : 1;        /** CPU sets this bit to one to indicate that the last exception
                                              * was taken while executing in a branch delay slot.
                                              *
                                              * source: L64360 datasheet
                                              */
        };

        uint32_t _reg;
    };

    // cop0r12 System status, rw
    union STATUS {
        enum class Mode : uint32_t { kernel = 0, user = 1 };
        enum class BootExceptionVectors { ram = 0, rom = 1 };
        struct {
            uint32_t interruptEnable : 1;
            Mode mode : 1;

            uint32_t previousInterruptEnable : 1;
            Mode previousMode : 1;

            uint32_t oldInterruptEnable : 1;
            Mode oldMode : 1;

            uint32_t : 2;

            uint32_t interruptMask : 8;
            uint32_t isolateCache : 1;
            uint32_t swappedCache : 1;
            uint32_t writeZeroAsParityBits : 1;
            uint32_t : 1;  // CM
            uint32_t cacheParityError : 1;
            uint32_t tlbShutdown : 1;  // TS

            BootExceptionVectors bootExceptionVectors : 1;
            uint32_t : 2;
            uint32_t reverseEndianness : 1;
            uint32_t : 2;

            uint32_t cop0Enable : 1;
            uint32_t cop1Enable : 1;
            uint32_t cop2Enable : 1;
            uint32_t cop3Enable : 1;
        };

        uint32_t _reg;
    };
}