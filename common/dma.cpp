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
};