#include <psx.h>
#include <stdio.h>
#include <stdlib.h>
#include "mdec.h"
#include "chrono/tables.h"
#include "chrono/frame.h"

int main() {
    mdec_reset();
    
    mdec_quantTable(quant, true);
    mdec_idctTable(idct);

    printf("before decode\n");
    mdec_decode((uint16_t*)macroblock, macroblock_len/2, bit_24, false, true);
    printf("after decode\n");

    //use dma

    int i = 0;
    printf("P3\n");
    printf("320 240\n");
    printf("255\n");
    while (!mdec_dataFifoEmpty()) {
        uint32_t pixel = mdec_read();
        printf("%d %d %d ", (pixel>>16)&0xff, (pixel>>8)&0xff, (pixel)&0xff);
        if (i % 320 == 0  && i > 0) {
            printf("\n");
        }
        i++;
    }
    printf("\n");
    return 0;
}