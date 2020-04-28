#include <common.h>
#include <dma.hpp>
#include <io.h>
#include <malloc.h>
#include <measure.hpp>
#include <string.h>
#include <test.h>

static const char *BoolToString(bool v) { return v ? "true" : "false"; }

static uint32_t GetAddress(const void *p) {
  return reinterpret_cast<uint32_t>(p) & 0x1FFFFFFFu;
}

volatile uint32_t g_dont_optimize_me = 0;

static uint32_t TimeWork() {
  return measure([]() {
    g_dont_optimize_me = 0;
    for (int i = 0; i < 1000; i++) {
      //__asm("nop\n");
      g_dont_optimize_me += i;
    }
  });
}

static void StartLinkedListDMA(uint32_t start_address) {
  write32(0x1F801814, 0x04000000);
  write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU, 0);
  write32(DMA::CH_BASE_ADDR + 0x10 * (int)DMA::Channel::GPU,
          DMA::MADDR(start_address)._reg);

  write32(0x1F801814, 0x04000002);
  DMA::channelIRQEnable(DMA::Channel::GPU, true);
  write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU,
          DMA::CHCR::GPULinkedList()._reg);
}

static bool IsGPUDMAFinished() {
  uint32_t value = read32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU);
  return (value & (1u << 24)) == 0;
}

static bool HasGPUDMAIRQ() { return DMA::channelIRQSet(DMA::Channel::GPU); }

static void StopLinkedListDMA() {
  write32(0x1F801814, 0x04000000);
  write32(DMA::CH_CONTROL_ADDR + 0x10 * (int)DMA::Channel::GPU, 0);
}

void TestNoDMA() {
  printf("Testing with no DMA active...\n");

  uint32_t ticks = TimeWork();
  printf("  Work took %u ticks\n", ticks);
}

void TestEmptyChain() {
  printf("Testing with empty but complete chain...\n");

  uint32_t chain[2] = {};
  chain[0] = GetAddress(&chain[1]);
  chain[1] = 0x00FFFFFFu;

  StartLinkedListDMA(GetAddress(&chain[0]));
  uint32_t ticks = TimeWork();
  bool was_finished = IsGPUDMAFinished();
  bool dma_irq = HasGPUDMAIRQ();
  StopLinkedListDMA();
  printf("  Work took %u ticks, finished = %s, irq = %s\n", ticks,
         BoolToString(was_finished), BoolToString(dma_irq));
}

void TestSingleSelfReferencingChain() {
  printf("Testing single self-referencing chain...\n");

  uint32_t chain[2] = {};
  chain[0] = GetAddress(&chain[0]);
  chain[1] = 0x00FFFFFFu;

  StartLinkedListDMA(GetAddress(&chain[0]));
  uint32_t ticks = TimeWork();
  bool was_finished = IsGPUDMAFinished();
  bool dma_irq = HasGPUDMAIRQ();
  StopLinkedListDMA();
  printf("  Work took %u ticks, finished = %s, irq = %s\n", ticks,
         BoolToString(was_finished), BoolToString(dma_irq));
}

void TestDoubleSelfReferencingChain() {
  printf("Testing double self-referencing chain...\n");

  uint32_t chain[3] = {};
  chain[0] = GetAddress(&chain[1]);
  chain[1] = GetAddress(&chain[0]);
  chain[2] = 0x00FFFFFFu;

  StartLinkedListDMA(GetAddress(&chain[0]));
  uint32_t ticks = TimeWork();
  bool was_finished = IsGPUDMAFinished();
  bool dma_irq = HasGPUDMAIRQ();
  StopLinkedListDMA();
  printf("  Work took %u ticks, finished = %s, irq = %s\n", ticks,
         BoolToString(was_finished), BoolToString(dma_irq));
}

int main() {
  initVideo(320, 240);
  printf("\ndma/chain-looping\n");
  printf("Tests CPU behavior with various invalid GPU linked list chains.\n\n");

  clearScreen();

  DMA::masterIRQEnable(true);
  DMA::masterEnable(DMA::Channel::GPU, true);

  EnterCriticalSection();
  TestNoDMA();
  TestEmptyChain();
  TestSingleSelfReferencingChain();
  TestDoubleSelfReferencingChain();
  ExitCriticalSection();

  DMA::masterEnable(DMA::Channel::GPU, false);

  for (;;) {
    VSync(false);
  }

  return 0;
}
