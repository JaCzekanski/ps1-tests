#include <common.h>
#include <test.h>
#include <dma.hpp>
#include <psxspu.h>
#include <string.h>
#include <io.h>
#include <measure.hpp>

const uint32_t SPU_TRANSFER_ADDRESS = 0x1F801DA6;
const uint32_t SPU_FIFO = 0x1F801DA8;
const uint32_t SPUCNT = 0x1F801DAA;
const uint32_t SPUSTAT = 0x1F801DAE;
const uint32_t SPUDTC = 0x1F801DAC;

namespace SPU {
    enum TransferMode {
        Stop = 0,
        ManualWrite,
        DMAWrite, 
        DMARead
    };
    bool setTransferMode(TransferMode mode) {
        uint16_t cnt = read16(SPUCNT);
        cnt &= ~(0b11 << 4); // Clear transfer mode bits
        cnt |= (uint16_t)mode << 4;
        write16(SPUCNT, cnt);

        // Wait for changes to be applied (SPUCNT bits 0..5 should be applied in SPUSTAT 0..5 after a while)
        for (int i = 0; i < 100; i++) {
            uint16_t stat = read16(SPUSTAT);

            if ((stat & 0x3f) == (cnt & 0x3f)) {
                return true;
            }
        }
        return false;
    }

    // Data Transfer Control - how data from FIFO is written to SPU RAM
    // dtc = 2 - normal write
    void setDTC(uint8_t dtc) {
        write16(SPUDTC, (dtc & 0b111) << 1);
    }

    // Address is divided by 8 internally
    void setStartAddress(uint32_t addr) {
        if (addr > 512*1024) {
            printf("\n[SPU] setStartAddress called with addr > SPU_RAM_SIZE\n");
        }
        write16(SPU_TRANSFER_ADDRESS, addr / 8);
    }

    bool waitForDMAready() {
        for (int i = 0; i < 100; i++) {
            if ((read16(SPUSTAT) & (1<<10)) == 0) {
                return true;
            }
        }
        printf("\n[SPU] SPUSTAT bit10 (DMA busy flag) is stuck high, fix your emulator\n");
        return false;
    }

    bool setupDMARead(uint32_t address, void* dst, size_t size, int BS = 0x10,
                      DMA::CHCR::SyncMode syncMode = DMA::CHCR::SyncMode::syncBlockToDmaRequests) {
        write32(0x1F801014, 0x220931E1);
        SPU::setDTC(2);
        SPU::setTransferMode(SPU::TransferMode::Stop);
        SPU::setStartAddress(address);
        SPU::setTransferMode(SPU::TransferMode::DMARead);
        SPU::waitForDMAready();

        const int BC = size / (4 * BS);
        write32(DMA::CH_BASE_ADDR    + 0x10 * (int)DMA::Channel::SPU, DMA::MADDR((uint32_t)dst)._reg);
        if (syncMode == DMA::CHCR::SyncMode::startImmediately) {
          const int WC = size / 4;
          write32(DMA::CH_BLOCK_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::BCR::mode0(WC)._reg);
        } else if (syncMode == DMA::CHCR::SyncMode::syncBlockToDmaRequests) {
          const int BC = size / (4 * BS);
          write32(DMA::CH_BLOCK_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::BCR::mode1(BS, BC)._reg);
        }
    }

    bool setupDMAWrite(uint32_t address, void* src, size_t size, int BS = 0x10,
                       DMA::CHCR::SyncMode syncMode = DMA::CHCR::SyncMode::syncBlockToDmaRequests) {
        SPU::setDTC(2);
        SPU::setTransferMode(SPU::TransferMode::Stop);
        SPU::setStartAddress(address);
        SPU::setTransferMode(SPU::TransferMode::DMAWrite);
        SPU::waitForDMAready();

        write32(DMA::CH_BASE_ADDR    + 0x10 * (int)DMA::Channel::SPU, DMA::MADDR((uint32_t)src)._reg);
        if (syncMode == DMA::CHCR::SyncMode::startImmediately) {
          const int WC = size / 4;
          write32(DMA::CH_BLOCK_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::BCR::mode0(WC)._reg);
        } else if (syncMode == DMA::CHCR::SyncMode::syncBlockToDmaRequests) {
          const int BC = size / (4 * BS);
          write32(DMA::CH_BLOCK_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::BCR::mode1(BS, BC)._reg);
        }
    }
    
    void startDMARead(DMA::CHCR::SyncMode syncMode = DMA::CHCR::SyncMode::syncBlockToDmaRequests) {
        write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::CHCR::SPUread(syncMode)._reg);
    }
    
    void startDMAWrite(DMA::CHCR::SyncMode syncMode = DMA::CHCR::SyncMode::syncBlockToDmaRequests) {
        write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::CHCR::SPUwrite(syncMode)._reg);
    }

    bool waitDmaTransferDone() {
        volatile DMA::CHCR* control = (DMA::CHCR*)(0x1F801088 + 0x10 * (int)DMA::Channel::SPU);

        for (int i = 0; i<10000; i++) {
            if (control->enabled == DMA::CHCR::Enabled::completed) {
                return true;
            }
        }

        return false;
    }
};

// Note: If this test is run in the first place than all read tests fail ?
void testControlBitsAreCopiedToStatusRegister() {
    auto test = [](uint16_t bits) {
        uint16_t cnt = bits;
        write16(SPUCNT, cnt);

        for (int i = 0; i < 100; i++) {
            uint16_t stat = read16(SPUSTAT);

            if ((stat & 0x3f) == (cnt & 0x3f)) {
                break;
            }
        }
        assertEquals(read16(SPUSTAT) & 0x3f, cnt & 0x3f);
    };

    TEST_MULTIPLE_BEGIN();
    test(0x3f);
    test(0x00);
    test(0x15);
    test(0x2a);
    TEST_MULTIPLE_END();
}

void testDtcRegister() {
    auto test = [](uint16_t bits) {
        write16(SPUDTC, bits);
        assertEquals(read16(SPUDTC), bits);
    };

    TEST_MULTIPLE_BEGIN();
    test(0x0000);
    test(0xffff);
    test(0x0000);
    test(0x5555);
    test(0x0000);
    test(0xaaaa);
    TEST_MULTIPLE_END();
}

const char* TEST_STRING = "Hello there   :)";

void testManualWriteToSpuRam() {
    SPU::setDTC(2);
    SPU::setTransferMode(SPU::TransferMode::Stop);
    SPU::setStartAddress(0x1000);

    for (int i = 0; i<strlen(TEST_STRING); i+=2) {
        write16(SPU_FIFO, TEST_STRING[i] | TEST_STRING[i+1] << 8);
    }

    SPU::setTransferMode(SPU::TransferMode::ManualWrite);

    for (int i = 0; i < 100; i++) {
        if ((read16(SPUSTAT) & (1<<10)) == 0) {
            break;
        }
    }

    assertEquals(read16(SPUSTAT) & (1<<10), 0);
}

// Note: Must be run after ManualWrite test, so that data is already in SPU RAM
void testDMAReadFromSpuRam() {
    char buf[32];
    for (int i = 0; i<32; i++) {
        buf[i] = 0xcc;
    }

    SPU::setupDMARead(0x1000, buf, strlen(TEST_STRING), 1);
    SPU::startDMARead();
    SPU::waitDmaTransferDone();
   
    TEST_MULTIPLE_BEGIN();
    for (int i = 0; i<strlen(TEST_STRING); i++) {
        assertEquals(buf[i], TEST_STRING[i]);
    }
    TEST_MULTIPLE_END();
}

void testDMAWriteToSpuRam() {
    SPU::setupDMAWrite(0x2000, (void*)TEST_STRING, strlen(TEST_STRING), 1);
    SPU::startDMAWrite();
    SPU::waitDmaTransferDone();
   
    // Assuming DMAread was already tested

    char buf[32];
    for (int i = 0; i<32; i++) {
        buf[i] = 0xcc;
    }

    SPU::setupDMARead(0x2000, buf, strlen(TEST_STRING), 1);
    SPU::startDMARead();
    SPU::waitDmaTransferDone();
   
    TEST_MULTIPLE_BEGIN();
    for (int i = 0; i<strlen(TEST_STRING); i++) {
        assertEquals(buf[i], TEST_STRING[i]);
    }
    TEST_MULTIPLE_END();
}

volatile DMA::CHCR* dmaSpuControl = (DMA::CHCR*)(0x1F801088 + 0x10 * (int)DMA::Channel::SPU);
// I'm very sorry for the usage of global variables here, but I don't have an access to std::function
// which prevents me from passing local references to lambda
int loopCount = 0;

void testDMAWriteTiming() {
    const size_t writeSize = 1024;
    uint8_t* buf = new uint8_t[writeSize];
    SPU::setupDMAWrite(0x1000, buf, writeSize);
    
    SPU::startDMAWrite();
    
    loopCount = 0;
    uint32_t measuredCycles = measure([] {
        for (; loopCount<10000; loopCount++) {
            if (dmaSpuControl->enabled == DMA::CHCR::Enabled::completed) {
                break;
            }
        }
    });

    TEST_MULTIPLE_BEGIN();

    assertEqualsWithComment(dmaSpuControl->enabled, DMA::CHCR::Enabled::completed, "DMA transfer was not completed, make sure you're setting bit24 (Transfer completed) in DMAn_CHCR register");

    bool transferFinishedImmediately = loopCount == 0;
    assertEqualsWithComment(transferFinishedImmediately, false, "DMA in mode 1 should allow for code execution between block transfers");
    
    // TODO: The transfer speed does not match those mentioned in NoCash docs (4cycles/word)
    assertEqualsWithComment(measuredCycles > writeSize * 16 * 0.1f, true, "DMA transfer was too fast");
    assertEqualsWithComment(measuredCycles < writeSize * 16 * 1.1f, true, "DMA transfer was too slow");
    
    TEST_MULTIPLE_END();

    delete[] buf;
}

// Note: Timing tests are almost identical, they could be refactored into single one
void testDMAReadTiming() {
    const size_t readSize = 1024;
    uint8_t* buf = new uint8_t[readSize];
    SPU::setupDMARead(0x1000, buf, readSize);
    
    SPU::startDMARead();
    
    loopCount = 0;
    uint32_t measuredCycles = measure([] {
        for (; loopCount<10000; loopCount++) {
            if (dmaSpuControl->enabled == DMA::CHCR::Enabled::completed) {
                break;
            }
        }
    });

    TEST_MULTIPLE_BEGIN();

    assertEqualsWithComment(dmaSpuControl->enabled, DMA::CHCR::Enabled::completed, "DMA transfer was not completed, make sure you're setting bit24 (Transfer completed) in DMAn_CHCR register");

    bool transferFinishedImmediately = loopCount == 0;
    assertEqualsWithComment(transferFinishedImmediately, false, "DMA in mode 1 should allow for code execution between block transfers");
    
    // TODO: The transfer speed does not match those mentioned in NoCash docs (4cycles/word)
    assertEqualsWithComment(measuredCycles > readSize * 16 * 0.1f, true, "DMA transfer was too fast");
    assertEqualsWithComment(measuredCycles < readSize * 16 * 1.1f, true, "DMA transfer was too slow");
    
    TEST_MULTIPLE_END();

    delete[] buf;
}

void testDMAWriteToRamSyncMode0() {
    SPU::setupDMAWrite(0x2100, (void*)TEST_STRING, strlen(TEST_STRING), 1, DMA::CHCR::SyncMode::startImmediately);
    SPU::startDMAWrite(DMA::CHCR::SyncMode::startImmediately);
    assertEqualsWithComment(SPU::waitDmaTransferDone(), true, "DMA write was completed");
}

void testDMAReadToRamSyncMode0() {
    // NOTE: Assumes testDMAWriteToRamSyncMode0 has run first.
  
    char download_buf[32] = {};
    SPU::setupDMARead(0x2100, download_buf, strlen(TEST_STRING), 1, DMA::CHCR::SyncMode::startImmediately);
    SPU::startDMARead(DMA::CHCR::SyncMode::startImmediately);

    TEST_MULTIPLE_BEGIN();
    assertEqualsWithComment(SPU::waitDmaTransferDone(), true, "DMA read was completed");
   
    for (size_t i = 0; i < strlen(TEST_STRING); i++) {
        assertEquals(download_buf[i], TEST_STRING[i]);
    }
    TEST_MULTIPLE_END();
}

void runTests() {
    testDtcRegister();
    testManualWriteToSpuRam();
    testDMAReadFromSpuRam();
    testDMAWriteToSpuRam();
    testDMAWriteTiming();
    testDMAReadTiming();
    testDMAWriteToRamSyncMode0();
    testDMAReadToRamSyncMode0();
    testControlBitsAreCopiedToStatusRegister();

    // TODO: Different DTC transfer behaviour
    
    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\nspu/memory-transfer\n");
    printf("Test SPU memory access using DMA and regular IO.\n\n");
	
    clearScreen();
    writeGP0(1, 0);

    SpuInit();
    DMA::masterEnable(DMA::Channel::SPU, true);

    EnterCriticalSection();
    runTests();
    ExitCriticalSection();

    for (;;) {
        VSync(false);
    }

    return 0;
}
