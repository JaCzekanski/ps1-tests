#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <psxgpu.h>
#include <psxapi.h>
#include <test.h>
#include <delay.h>
#include <io.h>
#include <dma.hpp>

using namespace DMA;

#define SCR_W 320
#define SCR_H 240

void writeGP0cpu(uint32_t raw) {
    volatile uint32_t *GP0 = (uint32_t*)0x1f801810;
    (*GP0) = raw;
}

uint8_t cmdGetSize(void* ptr) {
    return (((P_TAG*)ptr)->len);
}

__attribute__((noinline)) void drawPrimByCpu(void* p) {
    uint8_t len = cmdGetSize(p);
    uint32_t* ptr = ((uint32_t*)p);
    for (int i = 0; i < len; i++) {
        writeGP0cpu(*++ptr);
    }
}

void drawRect()
{
    TILE p;
    setTile(&p);
    setXY0(&p, 0, 0);
    setWH(&p, 128, 128);
    setRGB0(&p, 128, 0, 0);
    drawPrimByCpu(&p);
}

void setDithering(int dithering) 
{
    DR_TPAGE e;
    unsigned short texpage = getTPage(/* bitcount - do not care */0, /* semi-transparency mode */ 0, /*x*/0, /*y*/0);
    setDrawTPage(&e, /* Drawing to display area */ 1, dithering, texpage);
    DrawPrim(&e);
}

void waitForDma(Channel ch) {
    volatile CHCR* control = (CHCR*)(0x1F801088 + 0x10 * (int)ch);

    while (
        control->startTrigger == CHCR::StartTrigger::manual && 
        control->enabled != CHCR::Enabled::completed
    );
}

void setupDmaGpu(uint32_t address, BCR bcr, CHCR control) {
    MADDR addr;
    addr.address = address;

    waitForDma(Channel::GPU);

    write32(DMA::baseAddr(Channel::GPU),    addr._reg);
    write32(DMA::blockAddr(Channel::GPU),   bcr._reg);
    write32(DMA::controlAddr(Channel::GPU), control._reg);
}

#define assertBit(given, bit, state) assertEquals((given) & (1<<bit), state<<bit)

constexpr int DMA_REQUEST = 25;
constexpr int READY_TO_RECV_CMD_WORD = 26;
constexpr int READY_TO_SEND_VRAM = 27;
constexpr int READY_TO_RECV_DMA_BLOCK = 28;
constexpr int DMA_DIRECTION = 29;

void testGpustat_CPU_beforeRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       0);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 0);
    TEST_MULTIPLE_END();
}

void testGpustat_CPU_duringRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       0);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            0);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 0);
    TEST_MULTIPLE_END();
}

void testGpustat_CPU_afterRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       0);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 0);
    TEST_MULTIPLE_END();
}

void runTestRenderFromCpu() {
    VSync(0);
    DrawSync(0);
    
    EnterCriticalSection();

    writeGP1(4, 0); // DMA direction: 0 - off
    
    uint32_t gpustatBefore = ReadGPUstat();
    drawRect();
    uint32_t gpustatDuring = ReadGPUstat();
    delay(1000000);
    uint32_t gpustatAfter = ReadGPUstat();

    ExitCriticalSection();

    testGpustat_CPU_beforeRender(gpustatBefore);
    testGpustat_CPU_duringRender(gpustatDuring);
    testGpustat_CPU_afterRender(gpustatAfter);
}

void testGpustat_linkedListDMA_beforeRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 2);
    TEST_MULTIPLE_END();
}

void testGpustat_LinkedListDMA_duringRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       0);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            0);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           0);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 2);
    TEST_MULTIPLE_END();
}

void testGpustat_LinkedListDMA_afterRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 2);
    TEST_MULTIPLE_END();
}

void runTestRenderFromDmaLinkedList() {
    uint32_t renderList[32] = {0x00ffffff};
    TILE* p = (TILE*)renderList;
    setTile(p);
    setXY0(p, 128, 0);
    setWH(p, 128, 128);
    setRGB0(p, 0, 128, 0);

    renderList[0] = (cmdGetSize(p)<<24) | (((uint32_t)renderList + 4*4) & 0x7FFFFF); // Next block at index 4 (right after first command)
    
    p = (TILE*)(renderList + 4);
    setTile(p);
    setXY0(p, 256, 0);
    setWH(p, 128, 128);
    setRGB0(p, 0, 0, 128);

    renderList[4] = (cmdGetSize(p)<<24) | (0xffffff); // End marker (size 3)

    VSync(0);
    DrawSync(0);
    
    DMA::masterEnable(DMA::Channel::GPU, true);
    writeGP1(4, 2); // DMA direction: 2 - CPU to GP0

    uint32_t gpustatBefore = ReadGPUstat();
    setupDmaGpu((uint32_t)renderList, BCR(), CHCR::GPULinkedList());
    uint32_t gpustatDuring = ReadGPUstat();
    delay(100000);
    uint32_t gpustatAfter = ReadGPUstat();

    
    testGpustat_linkedListDMA_beforeRender(gpustatBefore);
    testGpustat_LinkedListDMA_duringRender(gpustatDuring);
    testGpustat_LinkedListDMA_afterRender(gpustatAfter);
}

void testGpustat_SyncModeDMA_dir1_beforeRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 1);
    TEST_MULTIPLE_END();
}

void testGpustat_SyncModeDMA_dir1_duringRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1); // ?
    assertBit(stat, READY_TO_RECV_CMD_WORD,            0);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           0);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 1);
    TEST_MULTIPLE_END();
}

void testGpustat_SyncModeDMA_dir1_afterRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 1);
    TEST_MULTIPLE_END();
}

void runTestRenderFromDmaSyncBlock_dir1() {
    uint32_t renderList[32] = {0x00ffffff};
    TILE* p = (TILE*)renderList;
    setTile(p);
    setXY0(p, 0, 128);
    setWH(p, 128, 128);
    setRGB0(p, 128, 128, 0);

    renderList[0] = 0; // Nop
    
    p = (TILE*)(renderList + 4);
    setTile(p);
    setXY0(p, 128, 128);
    setWH(p, 128, 128);
    setRGB0(p, 0, 128, 128);

    renderList[4] = 0; // Nop

    VSync(0);
    DrawSync(0);
    
    DMA::masterEnable(DMA::Channel::GPU, true);
    writeGP1(4, 1); // DMA direction: 1 - FIFO?

    uint32_t gpustatBefore = ReadGPUstat();
    setupDmaGpu((uint32_t)renderList, BCR::mode1(1, 8), CHCR::VRAMwrite());
    uint32_t gpustatDuring = ReadGPUstat();
    delay(100000);
    uint32_t gpustatAfter = ReadGPUstat();
    
    testGpustat_SyncModeDMA_dir1_beforeRender(gpustatBefore);
    testGpustat_SyncModeDMA_dir1_duringRender(gpustatDuring);
    testGpustat_SyncModeDMA_dir1_afterRender(gpustatAfter);
}

void testGpustat_SyncModeDMA_dir2_beforeRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 2);
    TEST_MULTIPLE_END();
}

void testGpustat_SyncModeDMA_dir2_duringRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       0); // ?
    assertBit(stat, READY_TO_RECV_CMD_WORD,            0);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           0);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 2);
    TEST_MULTIPLE_END();
}

void testGpustat_SyncModeDMA_dir2_afterRender(uint32_t stat) {
    TEST_MULTIPLE_BEGIN();
    assertBit(stat, DMA_REQUEST,                       1);
    assertBit(stat, READY_TO_RECV_CMD_WORD,            1);
    assertBit(stat, READY_TO_SEND_VRAM,                0);
    assertBit(stat, READY_TO_RECV_DMA_BLOCK,           1);
    assertEquals((stat & 0x60000000) >> DMA_DIRECTION, 2);
    TEST_MULTIPLE_END();
}

void runTestRenderFromDmaSyncBlock_dir2() {
    uint32_t renderList[32] = {0x00ffffff};
    TILE* p = (TILE*)renderList;
    setTile(p);
    setXY0(p, 0, 128);
    setWH(p, 128, 128);
    setRGB0(p, 128, 128, 0);

    renderList[0] = 0; // Nop
    
    p = (TILE*)(renderList + 4);
    setTile(p);
    setXY0(p, 128, 128);
    setWH(p, 128, 128);
    setRGB0(p, 0, 128, 128);

    renderList[4] = 0; // Nop

    VSync(0);
    DrawSync(0);
    
    DMA::masterEnable(DMA::Channel::GPU, true);
    writeGP1(4, 2); // DMA direction: 2 - CPUtoGP0

    uint32_t gpustatBefore = ReadGPUstat();
    setupDmaGpu((uint32_t)renderList, BCR::mode1(1, 8), CHCR::VRAMwrite());
    uint32_t gpustatDuring = ReadGPUstat();
    delay(100000);
    uint32_t gpustatAfter = ReadGPUstat();

    testGpustat_SyncModeDMA_dir2_beforeRender(gpustatBefore);
    testGpustat_SyncModeDMA_dir2_duringRender(gpustatDuring);
    testGpustat_SyncModeDMA_dir2_afterRender(gpustatAfter);
}


void runTests() {
    runTestRenderFromCpu();
    runTestRenderFromDmaLinkedList(); 
    runTestRenderFromDmaSyncBlock_dir1();
    runTestRenderFromDmaSyncBlock_dir2();

    printf("Done.\n");
}

int main()
{
    initVideo(SCR_W, SCR_H);
    printf("\ngpu/gpustat\n");

    printf("Bits in GPUSTAT description:\n");
    
    printf("Bit 25 - DMA Request \n");
    printf("  - When DMA Direction == 0 (Off) - always 0\n");
    printf("  - When DMA Direction == 1 (FIFO) - ?\n");
    printf("  - When DMA Direction == 2 (CPUtoGP0) - same as Bit 28\n");
    printf("  - When DMA Direction == 3 (GPUREADtoCPU) - same as Bit 27\n");

    printf("Bit 26 - Ready to receive Cmd Word\n");
    printf("Bit 27 - Ready to send VRAM to CPU\n");
    printf("Bit 28 - Ready to receive DMA Block\n");
    printf("Bits 29-30 - DMA Direction (0 - Off, 1 - FIFO, 2 - CPUtoGP0, 3 - GPUREADtoCPU)\n\n");

    clearScreenColor(0xff, 0xff, 0xff);
    setDithering(1);

    runTests();

    for (;;) {
        VSync(0);
    }
    return 0;
}
