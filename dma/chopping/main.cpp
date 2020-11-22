#include <common.h>
#include <psxapi.h>
#include <malloc.h>
#include <dma.hpp>
#include <measure.hpp>
#include <twister.h>
#include <test.h>
#include <io.h>

using namespace DMA;

uint8_t* buffer;
const int VRAM_WIDTH = 1024;
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const int W = 64;
const int H = 64;
const int TRANSFER_SIZE = W * H * 2;

int position = 0;

void beginCpuToVramTransfer() {
    writeGP1(1, 0); // Reset command buffer
    DrawSync(0); // Wait for completion
    
    uint16_t x = position % (VRAM_WIDTH / W);
    uint16_t y = position / (VRAM_WIDTH / W);

    CPU2VRAM buf;
    setcode(&buf, 0xA0); // CPU -> VRAM
    setlen(&buf, 3);
    setXY0(&buf, x * W, y * H);
    setWH(&buf, W, H);
    DrawPrim(&buf);

    writeGP1(4, 2); // DMA Direction - CPU -> VRAM

    position++;
}

CHCR control; // must be global for `measure`

void prepareSyncMode1(int blockSize) {
    beginCpuToVramTransfer();
  
    auto addr = MADDR((uint32_t)buffer);
    BCR block = BCR::mode1(blockSize, TRANSFER_SIZE / blockSize / 4);
    
    control = CHCR::VRAMwrite();
    control.syncMode = CHCR::SyncMode::syncBlockToDmaRequests;

    waitForChannel(Channel::GPU);

    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::GPU, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::GPU, block._reg);
}

void prepareSyncMode0() {
    beginCpuToVramTransfer();
  
    auto addr = MADDR((uint32_t)buffer);
    BCR block = BCR::mode0(TRANSFER_SIZE / 4);
    
    control = CHCR::VRAMwrite();
    control.syncMode = CHCR::SyncMode::startImmediately;

    waitForChannel(Channel::GPU);

    write32(CH_BASE_ADDR    + 0x10 * (int)Channel::GPU, addr._reg);
    write32(CH_BLOCK_ADDR   + 0x10 * (int)Channel::GPU, block._reg);
}

inline void startDmaTransfer() {
    write32(CH_CONTROL_ADDR + 0x10 * (int)Channel::GPU, control._reg);
}

void generateData() {
    for (int i = 0; i<TRANSFER_SIZE; i+=2) {
        uint32_t v = randomMT();
        buffer[i] = v & 0xff;
        buffer[i+1] = (v>>8) & 0xff;
    }
}

void runDma() {
    const auto cycles = measure<MeasureMethod::CpuClock>([]() {
        startDmaTransfer();
        waitForChannel(Channel::GPU);
    });

    uint32_t gpustat = ReadGPUstat();
    bool gpuBusy = !(gpustat & (1<<26));

    printf("took %5d CPU cycles. %s\n", cycles, gpuBusy ? "(GPU busy)" : "");
}

void runTests() {
    // SyncMode 1
    for (int blockSize = 1; blockSize <= 65; blockSize++) {
        if (blockSize == 65) blockSize = 128;
        generateData();
        prepareSyncMode1(blockSize);

        printf("DMA2, SyncMode: %d, %4d bytes, blockSize: %3d                            ... ", 
            (int)control.syncMode,
            TRANSFER_SIZE,
            blockSize
        );

        runDma();
    }
    
    // SyncMode 0 without chopping
    {
        generateData();
        prepareSyncMode0();

        printf("DMA2, SyncMode: %d, %4d bytes, no chopping                               ... ", 
            (int)control.syncMode,
            TRANSFER_SIZE
        );

        runDma();
    }

    // SyncMode 0 with chopping
    for (int dmaWindowSize = 0; dmaWindowSize < 8; dmaWindowSize++) {
        for (int cpuWindowSize = 0; cpuWindowSize < 8; cpuWindowSize++) {
            generateData();
            prepareSyncMode0();
            control.choppingEnable = 1;
            control.choppingDmaWindowSize = dmaWindowSize;
            control.choppingCpuWindowSize = cpuWindowSize;

            printf("DMA2, SyncMode: %d, %4d bytes, dmaWindow: %3d words, cpuWindow: %3d clks ... ", 
                (int)control.syncMode,
                TRANSFER_SIZE, 
                1 << control.choppingDmaWindowSize,
                1 << control.choppingCpuWindowSize
            );
            
            runDma();
        }
    }
    
    writeGP1(1, 0);
    DrawSync(0);
}

int main() {
    SetVideoMode(MODE_NTSC);
    initVideo(SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("\ndma/chopping (DMA Channel 2 CPU->VRAM, chopping test)\n");

    clearScreen();
    seedMT(1337);
    EnterCriticalSection();
    
    masterEnable(Channel::MDECin,  false);
    masterEnable(Channel::MDECout, false);
    masterEnable(Channel::GPU,     true);
    masterEnable(Channel::CDROM,   false);
    masterEnable(Channel::SPU,     false);
    masterEnable(Channel::PIO,     false);
    masterEnable(Channel::OTC,     false);
    
    buffer = (uint8_t*)malloc(TRANSFER_SIZE);

    runTests();

    printf("Done.\n");
    
    masterEnable(Channel::GPU,     false);
    ExitCriticalSection();

    for (;;) {
        VSync(false);
    }
    return 0;
}