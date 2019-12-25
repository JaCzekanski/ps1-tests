#include <dma.hpp>

void waitForDma(DMA::Channel ch);
void setupDmaOtc(uint32_t address, uint16_t wordCount, DMA::CHCR control);
void dmaMasterEnable(DMA::Channel ch, bool enabled);