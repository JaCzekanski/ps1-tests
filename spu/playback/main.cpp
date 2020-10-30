#include <common.h>
#include <dma.hpp>
#include <psxspu.h>
#include <string.h>
#include <io.h>
#include <psxapi.h>
#include <stdarg.h>
#include "spu-dump.h"

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

    bool setupDMARead(uint32_t address, void* dst, size_t size, int BS = 0x10) {
        write32(0x1F801014, 0x220931E1);
        SPU::setDTC(2);
        SPU::setTransferMode(SPU::TransferMode::Stop);
        SPU::setStartAddress(address);
        SPU::setTransferMode(SPU::TransferMode::DMARead);
        SPU::waitForDMAready();

        const int BC = size / (4 * BS);
        write32(DMA::CH_BASE_ADDR    + 0x10 * (int)DMA::Channel::SPU, DMA::MADDR((uint32_t)dst)._reg);
        write32(DMA::CH_BLOCK_ADDR   + 0x10 * (int)DMA::Channel::SPU, DMA::BCR::mode1(BS, BC)._reg);
    }

    bool setupDMAWrite(uint32_t address, void* src, size_t size, int BS = 0x10) {
        SPU::setDTC(2);
        SPU::setTransferMode(SPU::TransferMode::Stop);
        SPU::setStartAddress(address);
        SPU::setTransferMode(SPU::TransferMode::DMAWrite);
        SPU::waitForDMAready();

        const int BC = size / (4 * BS);
        write32(DMA::CH_BASE_ADDR    + 0x10 * (int)DMA::Channel::SPU, DMA::MADDR((uint32_t)src)._reg);
        write32(DMA::CH_BLOCK_ADDR   + 0x10 * (int)DMA::Channel::SPU, DMA::BCR::mode1(BS, BC)._reg);
    }
    
    void startDMARead() {
        write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::CHCR::SPUread()._reg);
    }
    
    void startDMAWrite() {
        write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::SPU, DMA::CHCR::SPUwrite()._reg);
    }

    void waitDmaTransferDone() {
        volatile DMA::CHCR* control = (DMA::CHCR*)(0x1F801088 + 0x10 * (int)DMA::Channel::SPU);

        for (int i = 0; i<10000; i++) {
            if (control->enabled == DMA::CHCR::Enabled::completed) {
                break;
            }
        }
    }
};

void delay() {
    for (int i = 0; i<32; i++) {
       asm("nop");
    }
}

extern "C" {
void W(uint32_t addr, uint16_t data) {
    delay();

    if (addr == SPUCNT) {
        // Dismiss dma write attempts
        const uint16_t MODE_MASK = 0x30;
        const uint16_t DMA_WRITE = 2;
        if (((data & MODE_MASK) >> 4) == DMA_WRITE) {
            return;
        }
    }
    write16(addr, data);
}

int bytesWritten  = 0;

void FIFO(int num, ...) {
	va_list valist;
	va_start(valist, num);
    for (int i = 0; i < num; i++) {
        write16(SPU_FIFO, va_arg(valist, int));

        bytesWritten++;
        if (bytesWritten % 32 == 0) {
            SPU::waitForDMAready();
        }
    }
    va_end(valist);
}

}

void interpreter() {
    for (uint32_t ptr = 0;ptr < sizeof(spu_dump_bin);) {
        uint8_t opcode = spu_dump_bin[ptr++];
        if (opcode == 'X') { 
            if (ptr>1)return;
        }else if (opcode == 'V') {
            VSync(0); 
        }else if (opcode == 'W') {
            uint32_t addr = 0;
            addr |= spu_dump_bin[ptr++];
            addr |= spu_dump_bin[ptr++]<<8;
            addr |= spu_dump_bin[ptr++]<<16;
            addr |= spu_dump_bin[ptr++]<<24;

            uint16_t data = 0;
            data |= spu_dump_bin[ptr++];
            data |= spu_dump_bin[ptr++]<<8;

            W(addr, data);
        }else if (opcode == 'F') {
            uint16_t size = 0;
            size |= spu_dump_bin[ptr++]; 
            size |= spu_dump_bin[ptr++]<<8;

            for (uint32_t i = 0; i<size; i++) {
                uint16_t data = 0;
                data |= spu_dump_bin[ptr++];
                data |= spu_dump_bin[ptr++]<<8;
                
                W(SPU_FIFO, data);

                bytesWritten++;
                if (bytesWritten % 32 == 0) {
                    SPU::waitForDMAready();
                }
            }
        } else {
            printf("Unknown opcode %c (%d) @ 0x%x, breaking\n", opcode, opcode, ptr-1);
            return;
        }
    }
}

int main() {
    initVideo(320, 240);
    printf("\nspu/playback\n");
    printf("Playback SPU log.\n");
	
    clearScreen();
    SpuInit();
    
    interpreter();
    printf("Done.\n");

    for (;;) {
        VSync(false);
    }

    return 0;
}