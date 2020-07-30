#include <common.h>
#include <psxapi.h>
#include <test.h>
#include <io.h>
#include "exception.h"
#include "cop0.h"

cop0::STATUS readCop0Status() {
    cop0::STATUS status;
    __asm__ __volatile__("mfc0 %0, $12" : "=r"(status._reg));
    return status;
}

void setCop0Status(cop0::STATUS status) {
    __asm__ __volatile__("mtc0 %0, $12" : "+r"(status._reg));
}

uint32_t cop0Opcode() {
    uint32_t val;
    __asm__ __volatile__("mfc0 %0, $15" : "=r"(val));
    return val;
}

void cop0InvalidOpcode() {
    __asm__ __volatile__(".word ((16 << 26) | (0x1f << 21))"); // cop0 0x1f
}

void cop1Opcode() {
    __asm__ __volatile__(".word (17 << 26)"); // cop1 000000
}

void cop2Opcode() {
    uint32_t val;
    __asm__ __volatile__("mfc2 %0, $1" : "=r"(val));
}

void cop2InvalidOpcode() {
    __asm__ __volatile__(".word ((18 << 26) | (0x1f << 21))"); // cop2 0x1f
}

void cop3Opcode() {
    __asm__ __volatile__(".word (19 << 26)"); // cop3 000000
}

template<int cop, int enabled>
void setCop() {
    EnterCriticalSection();
    
    auto status = readCop0Status();
    if constexpr (cop == 0) status.cop0Enable = enabled;
    else if constexpr (cop == 1) status.cop1Enable = enabled;
    else if constexpr (cop == 2) status.cop2Enable = enabled;
    else if constexpr (cop == 3) status.cop3Enable = enabled;

    setCop0Status(status);

    ExitCriticalSection();
}

template<int cop>
void enableCop() {
    setCop<cop, 1>();
}

template<int cop>
void disableCop() {
    setCop<cop, 0>();
}

// COP0
void testCop0Disabled() {
    disableCop<0>(); // cop0Enable toggles Kernel/User availability, cannot be disabled

    cop0Opcode();
    assertFalse(wasExceptionThrown());
}

void testCop0Enabled() {
    enableCop<0>();

    cop0Opcode();
    assertFalse(wasExceptionThrown());
}

void testCop0InvalidOpcode() {
    enableCop<0>();

    cop0InvalidOpcode();
    assertFalse(wasExceptionThrown()); // ????
} 

// COP1
void testCop1Disabled() {
    disableCop<1>();

    cop1Opcode();
    assertTrue(wasExceptionThrown());
}

void testCop1Enabled() {
    enableCop<1>();

    cop1Opcode();
    assertFalse(wasExceptionThrown());
}

// COP2
void testCop2Disabled() {
    disableCop<2>();

    cop2Opcode();
    assertTrue(wasExceptionThrown());
}

void testCop2Enabled() {
    enableCop<2>();

    cop2Opcode();
    assertFalse(wasExceptionThrown());
}

void testCop2InvalidOpcode() {
    enableCop<2>();

    cop2InvalidOpcode();
    assertFalse(wasExceptionThrown()); // ?? It should throw?
}

// COP3
void testCop3Disabled() {
    disableCop<3>();

    cop3Opcode();
    assertTrue(wasExceptionThrown());
}

void testCop3Enabled() {
    enableCop<3>();

    cop3Opcode();
    assertFalse(wasExceptionThrown());
}

void testDisabledCoprocessorThrowsCoprocessorUnusable() {
    using Exception = cop0::CAUSE::Exception;
    disableCop<3>();

    cop3Opcode();
    assertEquals(getExceptionType(), Exception::coprocessorUnusable);
}


void runTests() {
    testCop0Disabled();
    testCop0Enabled();
    testCop0InvalidOpcode();

    testCop1Disabled();
    testCop1Enabled();

    testCop2Disabled();
    testCop2Enabled();
    testCop2InvalidOpcode();

    testCop3Disabled();
    testCop3Enabled();

    testDisabledCoprocessorThrowsCoprocessorUnusable();

    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\ncpu/cop\n");

    clearScreen();

    EnterCriticalSection();
    hookUnresolvedExceptionHandler(exceptionHandler);
    ExitCriticalSection();

    wasExceptionThrown(); //Just to clear the flag 
    runTests();
    
    for (;;) {
        VSync(false);
    }
    return 0;
}