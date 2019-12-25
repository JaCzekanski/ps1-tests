#include <common.h>
#include <psxapi.h>
#include <malloc.h>
#include <dma.hpp>
#include <test.h>
#include <io.h>
#include "otc.h"

using namespace DMA;

constexpr uint32_t DMA_OTC_CTRL = DMA::CH_CONTROL_ADDR + 0x10 * (int)Channel::OTC;

// Check DMA6 OTC with standard parameters
// DMA should create a reverse linked list where nth item points to n-1 item
// and item 0 is terminated with 0xffffff
void testOtcStandard() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    auto control = CHCR::OTC();
    setupDmaOtc((uint32_t)&buf[3], 4, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], ((uint32_t)&buf[2]) & 0xffffff);
    assertEquals(buf[2], ((uint32_t)&buf[1]) & 0xffffff);
    assertEquals(buf[1], ((uint32_t)&buf[0]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();
}

// Check Transfer with size == 0 
void testOtcBigTransfer() {
    const int SIZE = 64 * 1024;
    uint32_t* buf = (uint32_t*)malloc(sizeof(uint32_t) * SIZE);
    for (int i = 0; i<SIZE; i++) buf[i] = i;

    auto control = CHCR::OTC();
    setupDmaOtc((uint32_t)&buf[SIZE - 1], 0, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[SIZE - 1], ((uint32_t)&buf[SIZE - 2]) & 0xffffff);
    assertEquals(buf[SIZE - 2], ((uint32_t)&buf[SIZE - 3]) & 0xffffff);
    assertEquals(buf[1],        ((uint32_t)&buf[0])        & 0xffffff);
    assertEquals(buf[0],        0xffffff);
    TEST_MULTIPLE_END();

    free(buf);
}

// Check if DMA transfer doesn't start with StartTrigger bit (28) cleared
void testOtcWontStartOnAutomaticMode() {
    volatile uint32_t buf = 0x11111111;

    auto control = CHCR::OTC();
    control.startTrigger = CHCR::StartTrigger::automatic;
    setupDmaOtc((uint32_t)&buf, 1, control);

    assertEquals(buf, 0x11111111);
}

// Check if DMA transfer doesn't start with enabled bit (24) cleared
void testOtcWontStartWithoutStartFlag() {
    volatile uint32_t buf = 0x11111111;

    auto control = CHCR::OTC();
    control.enabled = CHCR::Enabled::stop;
    setupDmaOtc((uint32_t)&buf, 1, control);

    assertEquals(buf, 0x11111111);
}

// Check if reverse direction (From RAM) is ignored
void testOtcFromRam() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    auto control = CHCR::OTC();
    control.direction = CHCR::Direction::fromRam;
    setupDmaOtc((uint32_t)&buf[3], 4, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], ((uint32_t)&buf[2]) & 0xffffff);
    assertEquals(buf[2], ((uint32_t)&buf[1]) & 0xffffff);
    assertEquals(buf[1], ((uint32_t)&buf[0]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();
}

// Check if memoryAddressStep == Forward bit is ignored
void testOtcForward() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    auto control = CHCR::OTC();
    control.memoryAddressStep = CHCR::MemoryAddressStep::forward;
    setupDmaOtc((uint32_t)&buf[3], 4, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], ((uint32_t)&buf[2]) & 0xffffff);
    assertEquals(buf[2], ((uint32_t)&buf[1]) & 0xffffff);
    assertEquals(buf[1], ((uint32_t)&buf[0]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();
}

// Check if using SyncMode syncBlockToDmaRequests is allowed
void testOtcSyncModeBlocks() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    auto control = CHCR::OTC();
    control.syncMode = CHCR::SyncMode::syncBlockToDmaRequests;
    setupDmaOtc((uint32_t)&buf[3], 4, control);
    waitForDma(Channel::OTC);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], ((uint32_t)&buf[2]) & 0xffffff);
    assertEquals(buf[2], ((uint32_t)&buf[1]) & 0xffffff);
    assertEquals(buf[1], ((uint32_t)&buf[0]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();
}

// Check if using SyncMode Linked List is allowed
void testOtcSyncModeLinkedList() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    auto control = CHCR::OTC();
    control.syncMode = CHCR::SyncMode::linkedListMode;
    setupDmaOtc((uint32_t)&buf[3], 4, control);
    waitForDma(Channel::OTC);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], ((uint32_t)&buf[2]) & 0xffffff);
    assertEquals(buf[2], ((uint32_t)&buf[1]) & 0xffffff);
    assertEquals(buf[1], ((uint32_t)&buf[0]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();
}

// Check if using SyncMode Reserved is allowed
void testOtcSyncModeReserved() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    auto control = CHCR::OTC();
    control.syncMode = CHCR::SyncMode::reserved;
    setupDmaOtc((uint32_t)&buf[3], 4, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], ((uint32_t)&buf[2]) & 0xffffff);
    assertEquals(buf[2], ((uint32_t)&buf[1]) & 0xffffff);
    assertEquals(buf[1], ((uint32_t)&buf[0]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();
}

// Note: bit 24 Enabled (Start/Busy in Nocash docs) is cleared here 
// to prevent DMA from running
void testOtcWhichBitsAreHardwiredToZero() {
    const uint32_t control = 0b01110000'01110111'00000111'00000011;

    write32(DMA_OTC_CTRL, control);

    assertEquals(read32(DMA_OTC_CTRL), 0b01010000'00000000'00000000'00000010);
}

void testOtcWhichBitsAreHardwiredToOne() {
    const uint32_t control = 0b00000000'00000000'00000000'00000000;

    write32(DMA_OTC_CTRL, control);

    assertEquals(read32(DMA_OTC_CTRL), 0b00000000'00000000'00000000'00000010);
}

// Check if reading from unused bits returns 0
// Note: Bit1 Memory Address Step is hardwired for Backwards operation
void testOtcUnusedControlBitsAreAlwaysZero() {
    const uint32_t control = 0b10001110'10001000'11111000'11111100;

    write32(DMA_OTC_CTRL, control);

    assertEquals(read32(DMA_OTC_CTRL), 0b00000000'00000000'00000000'00000010);
}

// Check Control register contents right after running DMA
void testOtcControlBitsAfterTransfer() {
    const int SIZE = 32 * 1024;
    uint32_t* buf = (uint32_t*)malloc(sizeof(uint32_t) * SIZE);

    auto control = CHCR::OTC();
    setupDmaOtc((uint32_t)&buf[SIZE - 1], SIZE, control);

    assertEquals(read32(DMA_OTC_CTRL), 0b00000000'00000000'00000000'00000010);

    free(buf);
}

// Chopping is not supported
void testOtcControlBitsAfterTransferWithChopping() {
    const int SIZE = 32 * 1024;
    uint32_t* buf = (uint32_t*)malloc(sizeof(uint32_t) * SIZE);

    auto control = CHCR::OTC();
    control.choppingCpuWindowSize = 1;
    control.choppingDmaWindowSize = 1;
    control.choppingEnable = true;
    setupDmaOtc((uint32_t)&buf[SIZE - 1], SIZE, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(read32(DMA_OTC_CTRL), 0b00000000'00000000'00000000'00000010);
    assertEquals(buf[SIZE - 1], ((uint32_t)&buf[SIZE - 2]) & 0xffffff);
    assertEquals(buf[0], 0xffffff);
    TEST_MULTIPLE_END();

    waitForDma(Channel::OTC);
    free(buf);
}

void testOtcStandardWithMasterDisabled() {
    volatile uint32_t buf[4] = {
        0x11111111,
        0x22222222,
        0x33333333,
        0x44444444
    };

    dmaMasterEnable(Channel::OTC, false);

    auto control = CHCR::OTC();
    setupDmaOtc((uint32_t)&buf[3], 4, control);

    TEST_MULTIPLE_BEGIN();
    assertEquals(buf[3], 0x44444444);
    assertEquals(buf[2], 0x33333333);
    assertEquals(buf[1], 0x22222222);
    assertEquals(buf[0], 0x11111111);
    TEST_MULTIPLE_END();

    dmaMasterEnable(Channel::OTC, true);
}

void runTests() {
    testOtcStandard();
    testOtcBigTransfer();
    testOtcWontStartOnAutomaticMode();
    testOtcWontStartWithoutStartFlag();
    testOtcForward();
    testOtcFromRam();
    testOtcSyncModeBlocks();
    testOtcSyncModeLinkedList();
    testOtcSyncModeReserved();
    testOtcWhichBitsAreHardwiredToZero();
    testOtcWhichBitsAreHardwiredToOne();
    testOtcUnusedControlBitsAreAlwaysZero();
    testOtcControlBitsAfterTransfer();
    testOtcControlBitsAfterTransferWithChopping();
    testOtcStandardWithMasterDisabled();

    printf("Done.\n");
}


int main() {
    initVideo(320, 240);
    printf("\ndma/otc-test (DMA Channel 6)\n");

    clearScreen();
    EnterCriticalSection();
    
    dmaMasterEnable(Channel::MDECin,  false);
    dmaMasterEnable(Channel::MDECout, false);
    dmaMasterEnable(Channel::GPU,     false);
    dmaMasterEnable(Channel::CDROM,   false);
    dmaMasterEnable(Channel::SPU,     false);
    dmaMasterEnable(Channel::PIO,     false);
    dmaMasterEnable(Channel::OTC,     true);
    runTests();

    ExitCriticalSection();
    for (;;) {
        VSync(false);
    }
    return 0;
}