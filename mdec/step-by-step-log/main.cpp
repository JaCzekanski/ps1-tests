#include <common.h>
#include <psxsio.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <delay.h>
#include <io.h>
#include <dma.hpp>
#include <mdec.h>
#include <hexdump.h>
#include <gpu.h>
#include "symbols.h"

extern void mdec_data(uint32_t cmd);
int main() {
    // Use UART on PSX for faster printing
    AddSIO(115200);
    initVideo(320, 240);
    printf("\nmdec/step-by-step-log\n");
    printf("Manually feeds the MDEC data, monitors status register at every step.\n");
    printf("When MDECin FIFO is full, program switches to read mode which dumps decoded image until FIFO out gets empty. \n");
    printf("Decoded blocks are copied manually WITHOUT swizzling, blocks are copied as 16x4 instead of 8x8\n");

    clearScreen();

    mdec_reset();
    mdec_quantTable(quant, true);
    mdec_idctTable((int16_t*)idct);

    mdec_enableDma();

    const ColorDepth depth = ColorDepth::bit_15;
    const bool outputSigned = false;
    const bool setBit15 = false;

    printf("mdec_decode(addr=0x%08x, length=0x%x, colorDepth=%d, outputSigned=%d, setBit15=%d)... \n", symbols_mdec, symbols_mdec_len, depth, outputSigned, setBit15);
    size_t lengthWords = symbols_mdec_len / 4;
    uint32_t cmd = (1<<29) | (depth<<27) | (outputSigned<<26) | (setBit15 << 25) | (lengthWords & 0xffff);

    while (mdec_cmdBusy());
    mdec_cmd(cmd);

    // Warning: W, H must be set to correct dimensions
    int W = 16;
    int H = 64;

    const int bytesToRead = W * H * 2;
    int bytesToReadRemaining = bytesToRead;
    uint32_t image[bytesToRead / 4];
    int imagePtr = 0;

    uint16_t* rlePtr = (uint16_t*)symbols_mdec;

    auto printfStatus = []() {
        uint32_t status = read32(MDEC_STATUS);
        printf("==== MDEC_STATUS: 0x%08x, "
            "dataOutFifoEmpty: %d, "
            "dataInFifoFull: %d, " 
            "cmdBusy: %d, "
            "dataInReq: %d, "
            "dataOutReq: %d, "
            "currentBlock: %d\n", 
            status, 
            (status & (1 << 31)) != 0,
            (status & (1 << 30)) != 0,
            (status & (1 << 29)) != 0,
            (status & (1 << 28)) != 0,
            (status & (1 << 27)) != 0,
            (status >> 16) & 7
        );
    };

    while (true) {
        bool waitForImage = false;
        printf("\n\n----- MDECin\n");
        for (;lengthWords != 0;lengthWords--) {
            printfStatus();
            if (mdec_dataInFifoFull()) {
                printf("MDEC: dataInFifoFull, suspending 'mdecIn'\n");
                break;
            }
            uint32_t word = *rlePtr++;
            word |= ((uint32_t)(*rlePtr++)) << 16;

            mdec_data(word);
            printf("MDEC_DATA <- 0x%08x\n", word);
            delay(10000);

            waitForImage = true;
        }
        if (waitForImage) {
            while (mdec_dataOutFifoEmpty());
        }

        printf("\n\n----- MDECout\n");
        for (; bytesToReadRemaining != 0; bytesToReadRemaining -= 4) {
            printfStatus();
            if (mdec_dataOutFifoEmpty()) {
                printf("MDEC: dataOutFifoEmpty, suspending 'mdecOut'\n");
                break;
            }
            uint32_t read = mdec_read();
            printf("MDEC_DATA -> 0x%08x\n", read);
            image[imagePtr++] = read;
            delay(10000);
        }

        if (lengthWords <= 0 && bytesToReadRemaining <= 0) {
            break;
        }
    }

    printfStatus();
    printf("done\n");

    hexdump((uint8_t*)image, bytesToRead, 32);
    writeGP0(1, 0);
    vramWriteDMA(0, 0, W, H, (uint16_t*)image);
    
    // Stop all pending DMA transfers
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::MDECin, 0);
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::MDECout, 0);
    write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU, 0);
    DMA::masterEnable(DMA::Channel::MDECout, false);
    DMA::masterEnable(DMA::Channel::MDECin, false);
    DMA::masterEnable(DMA::Channel::GPU, false);

    mdec_reset();
    printf("Done\n");
    for (;;) {
        VSync(0);
    }
    return 0;
}