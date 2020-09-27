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

const char* TEST_STRING = "Hello there   :)";

void writeToSPURAM() {
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

void testSPUDMARead(bool dpcr_enable, unsigned priority) {
    printf("Testing DMA read with master enable = %s, priority = %u\n",
        dpcr_enable ? "true" : "false", priority);

    volatile uint32_t* dpcr = (volatile uint32_t*)0x1F8010F0;
    const uint32_t old_dpcr = *dpcr;
    const uint32_t test_dpcr = (old_dpcr & ~(uint32_t)0xF0000) | (((unsigned)dpcr_enable) << 19) | (priority << 16);
    printf("  DPCR was 0x%08X, setting to 0x%08X\n", old_dpcr, test_dpcr);
    *dpcr = test_dpcr;

    char buf[32];
    for (int i = 0; i<32; i++) {
        buf[i] = 0xcc;
    }

    SPU::setupDMARead(0x1000, buf, strlen(TEST_STRING), 1);
    SPU::startDMARead();
    if (!SPU::waitDmaTransferDone())
    {
      printf("  ... transfer timed out\n");
      *dpcr = old_dpcr;
      return;
    }

    printf("  ... transfer complete\n");
    *dpcr = old_dpcr;
   
    TEST_MULTIPLE_BEGIN();
    for (int i = 0; i<strlen(TEST_STRING); i++) {
        assertEquals(buf[i], TEST_STRING[i]);
    }
    TEST_MULTIPLE_END();
}


void runTests() {
    writeToSPURAM();
    testSPUDMARead(true, 5);
    testSPUDMARead(false, 5);
    testSPUDMARead(false, 0);
    
    printf("Done.\n");
}

int main() {
    initVideo(320, 240);
    printf("\ndma/dpcr\n");
    printf("Tests DMA transfer behaviour with various DPCR values.\n\n");
	
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

