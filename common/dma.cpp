#include "dma.hpp"
#include "io.h"

namespace DMA {
void waitForChannel(Channel ch) {
    volatile CHCR* control = (CHCR*)(0x1F801088 + 0x10 * (int)ch);

    while (control->enabled != CHCR::Enabled::completed);
}

void masterEnable(Channel ch, bool enabled) {
    uint32_t dmaControl = read32(CONTROL_ADDR);
    uint32_t mask = 0b1000 << ((int)ch * 4);
    if (enabled) {
        dmaControl |= mask;
    } else {
        dmaControl &= ~mask;
    }
    write32(CONTROL_ADDR, dmaControl);
}

void channelIRQEnable(Channel ch, bool enabled) {
    uint32_t dicr = read32(0x1F8010F4);
    if (enabled)
      dicr |= (1u << ((int)ch + 16)) + (1u << ((int)ch + 24));
    else
      dicr &= ~(1u << ((int)ch + 16));

    write32(0x1F8010F4, dicr);
}

void masterIRQEnable(bool enabled) {
    uint32_t dicr = read32(0x1F8010F4);
    if (enabled)
      dicr |= (1u << 23);
    else
      dicr &= ~(1u << 23);

    write32(0x1F8010F4, dicr);
}

bool channelIRQSet(Channel ch) {
  uint32_t dicr = read32(0x1F8010F4);
  return (dicr & (1u << ((int)ch + 24))) != 0u;
}

};
