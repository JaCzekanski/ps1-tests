#include <common.h>
#include <io.h>
#include <dma.hpp>
#include <measure.hpp>
#include <malloc.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <psxspu.h>

#define SCR_W 320
#define SCR_H 240

#define OT_LEN 2
#define PACKET_LEN 65535

struct DB {
    DISPENV disp;
    DRAWENV draw;
    uint32_t ot[OT_LEN];
    uint32_t ptr[PACKET_LEN];
};

DB db[2]; 
int activeDb = 0; // Double buffering index
uint32_t* ptr;

char padBuffer[2][34];
unsigned short buttons = 0xffff, prevButtons = 0xffff;

bool BUTTON(uint16_t button) {
	return (buttons & button) == 0 && ((prevButtons & button) != 0);
}

void init() {
    ResetGraph(0);
    SetVideoMode(MODE_NTSC);
    for (int i = 0; i<=1; i++) {
        SetDefDispEnv(&db[i].disp, 0, !i ? 0 : SCR_H, SCR_W, SCR_H);
        SetDefDrawEnv(&db[i].draw, 0, !i ? SCR_H : 0, SCR_W, SCR_H);

        db[i].disp.isinter = false; // Progressive mode
        db[i].disp.isrgb24 = false; // 16bit mode

        db[i].draw.dtd = false; // Disable dither
        db[i].draw.isbg = true; // Clear bg on PutDrawEnv
        setRGB0(&db[i].draw, 0, 0, 0);

        ClearOTagR(db[i].ot, OT_LEN);
    }
    activeDb = 0;

	PutDrawEnv(&db[activeDb].draw);
    ptr = db[activeDb].ptr;

    SetDispMask(1);

	InitPAD(padBuffer[0], 34, padBuffer[1], 34);
	StartPAD();
	ChangeClearPAD(0);

	FntLoad(960, 0);
	FntOpen(16, 8, SCR_W-8, SCR_H-8, 0, 2048);	
}

void swapBuffers() {
    FntFlush(-1);

    DrawSync(0); // Wait for GPU to finish rendering 
    VSync(0);

    activeDb = !activeDb;
    ptr = db[activeDb].ptr;

    ClearOTagR(db[activeDb].ot, OT_LEN);

    PutDrawEnv(&db[activeDb].draw);
    PutDispEnv(&db[activeDb].disp);

    DrawOTag(&db[1 - activeDb].ot[OT_LEN - 1]);

	prevButtons = buttons;
	buttons = ((PADTYPE*)padBuffer[0])->btn;
}

const uint32_t SPU_TRANSFER_ADDRESS = 0x1F801DA6;
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

        printf("\n[SPU] SPUCNT 0..5 bits are not visible in SPUSTAT 0..5, fix your emulator\n");
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
};

bool waitForSpuDmaChannel() {
    volatile DMA::CHCR* control = (DMA::CHCR*)(0x1F801088 + 0x10 * (int)DMA::Channel::SPU);

    int i = 0;
    bool dmaTransferCompleted = false;
    for (; i<10000; i++) {
        if (control->enabled == DMA::CHCR::Enabled::completed) {
            dmaTransferCompleted = true;
            break;
        }
    }

    if (!dmaTransferCompleted) {
        printf("\n[DMA] DMA transfer was not completed, make sure you're setting bit24 (Transfer completed) in DMAn_CHCR register\n");
        return false;
    }
    if (i == 0) {
        printf("\n[DMA] Your emulator executes DMA immediately - it should allow for code execution between block transfers\n");
        return false;
    }
    printf("\n[DMA] DMA transfer took %d loop cycles\n", i);
    return true;
}

uint32_t measuredCpuCycles = 0;
bool testFailed = false;

#define CHECK(x) (testFailed |= !(x))

bool writeDataToSpuDMA(uint8_t* buf, uint32_t addr, size_t size) {
    testFailed = false;

    write32(0x1F801014, 0x200931e1);
    SPU::setDTC(2);
    CHECK(SPU::setTransferMode(SPU::TransferMode::Stop));
    SPU::setStartAddress(addr);
    CHECK(SPU::setTransferMode(SPU::TransferMode::DMAWrite));
    CHECK(SPU::waitForDMAready());

    using namespace DMA;
    const int BS = 0x10;
    const int BC = size / 4 / BS;

    write32(CH_BASE_ADDR  + 0x10 * (int)Channel::SPU, MADDR((uint32_t)buf)._reg);
    write32(CH_BLOCK_ADDR + 0x10 * (int)Channel::SPU, BCR::mode1(BS, BC)._reg);
    
    measuredCpuCycles = measure([] {
        write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::SPU, CHCR::SPUwrite()._reg);
        CHECK(waitForSpuDmaChannel());
    });

    return !testFailed;
}

bool readDataFromSpuDMA(uint8_t* dst, uint32_t addr, size_t size) {
    testFailed = false;

    write32(0x1F801014, 0x220931E1);
    SPU::setDTC(2);
    CHECK(SPU::setTransferMode(SPU::TransferMode::Stop));
    SPU::setStartAddress(addr);
    CHECK(SPU::setTransferMode(SPU::TransferMode::DMARead));
    CHECK(SPU::waitForDMAready());

    using namespace DMA;
    const int BS = 0x10;
    const int BC = size / 4 / BS;

    write32(CH_BASE_ADDR  + 0x10 * (int)Channel::SPU, MADDR((uint32_t)dst)._reg);
    write32(CH_BLOCK_ADDR + 0x10 * (int)Channel::SPU, BCR::mode1(BS, BC)._reg);
    
    measuredCpuCycles = measure([] {
        write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::SPU, CHCR::SPUread()._reg);
        CHECK(waitForSpuDmaChannel());
    });

    return !testFailed;
}

bool readDataFromSpu(uint8_t* dst, uint32_t addr, size_t size) {
    write32(0x1F801014, 0x220931E1);
    SPU::setDTC(2);
    SPU::setTransferMode(SPU::TransferMode::Stop);
    SPU::setStartAddress(addr);
    SPU::setTransferMode(SPU::TransferMode::ManualWrite);

    for (int i = 0; i<size; i++) {
        *(dst+i) = read8(0x1F801DA8);
    }
    return true;
}

char toChar(uint8_t c) {
    if (c >= 32 && c < 127) return (char)c;
    else return '.';
}

void hexdump(uint8_t* buf, size_t size, uint32_t displayStartAddress) {
    const int W = 16;
    
    for (int y = 0; y < size/W; y++) {
        printf("0x%08x: ", displayStartAddress + y * W);
        for (int x = 0; x<W; x++) {
            int i = y * W + x;
            if (i >= size) break;
            
            printf("%02x ", buf[i]);
        }

        for (int x = 0; x<W; x++) {
            int i = y * W + x;
            if (i >= size) break;

            printf("%c", toChar(buf[i]));
        }
        printf("\n");
    }
    printf("\n");
}

const char* expectedWriteTime(size_t count, uint32_t actual) {
    int eps = 512;
    int expected = count * 8;
    if (expected - actual > eps) return "too fast";
    else if (expected - actual < -eps) return "too slow";
    else return "ok";
}

int main() {
    init();
    printf("\nspu/dma-read\n");
    printf("This is a sandbox for checking SPU RAM reading and writing.\n");
    printf("Nothing is displayed on the screen - everything is output to the console.\n");
    printf("Controller mapping:\n");
    printf("< - transferSize -= 0x100\n");
    printf("> - transferSize += 0x100\n");
    printf("^ - startAddress += 0x100\n");
    printf("v - startAddress -= 0x100\n");
    printf("^ - Buffer hexdump\n");
    printf("█ - Read data from SPU RAM to Main RAM (IO, shouldn't work on PSX)\n");
    printf("O - Read data from SPU RAM to Main RAM (DMA)\n");
    printf("X - Write data to SPU RAM (DMA)\n");
	DrawSync(0);

    SpuInit();

    DMA::masterEnable(DMA::Channel::SPU, true);

    uint32_t startAddress = 0x0000;
    size_t transferSize = 0x100;

    uint8_t* buf = (uint8_t*)malloc(512*1024 * sizeof(uint8_t));
    if (!buf) {
        printf("[ERROR] Cannot alloc 512kB for SPU buffer\n");
    }

    for (;;) {
		if (BUTTON(PAD_LEFT)) {
            if (transferSize > 0x100) transferSize -= 0x100;
            printf("transferSize: %d bytes\n", transferSize);
        }
		if (BUTTON(PAD_RIGHT)) {
            transferSize += 0x100;
            printf("transferSize: %d bytes\n", transferSize);
        }
		if (BUTTON(PAD_UP)) {
            startAddress += 0x100;
            printf("startAddress: 0x%0x\n", startAddress);
        }
		if (BUTTON(PAD_DOWN)) {
            if (startAddress > 0) startAddress -= 0x100;
            printf("startAddress: 0x%0x\n", startAddress);
        }
		if (BUTTON(PAD_CROSS)) {
            // Write to SPU RAM using DMA
            
            for (int i = 0; i<transferSize; i++) {
                *(buf+i) = i;
                
                if (i % 256 == 0) *(buf+i) = i / 256;
            }

            printf("Writing %d bytes to SPU RAM to 0x%08x using DMA... ", transferSize, startAddress);
            bool result = writeDataToSpuDMA(buf, startAddress, transferSize);
            if (!result) {
                printf("fail\n");
            } else {
                printf("ok (took %d CPU cycles)\n", measuredCpuCycles);
            }
        }
		if (BUTTON(PAD_CIRCLE)) {
            // Read from SPU RAM using DMA
            
            // Clear buffer
            for (int i = 0 ; i < transferSize; i++) {
                *(buf+i) = 0xdd;
            }

            printf("Reading %d bytes from SPU RAM from 0x%08x using DMA... ", transferSize, startAddress);
            bool result = readDataFromSpuDMA(buf, startAddress, transferSize);
            if (!result) {
                printf("fail\n");
            } else {
                printf("(took %d CPU cycles)\n", measuredCpuCycles);
            }
        }
		if (BUTTON(PAD_SQUARE)) {
            // Read from SPU RAM using IO
            
            // Clear buffer
            for (int i = 0 ; i < transferSize; i++) {
                *(buf+i) = 0xdd;
            }

            printf("Reading %d bytes from SPU RAM from 0x%08x using IO (shouldn't work on PS1)... ", transferSize, startAddress);
            readDataFromSpu(buf, startAddress, transferSize);
            printf("ok\n");
        }
		if (BUTTON(PAD_TRIANGLE)) {
            // Dump buffer
            hexdump(buf, transferSize, startAddress);
        }

        swapBuffers();    
    }

    return 0;
}
