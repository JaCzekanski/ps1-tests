#include "hexdump.h"
#include <stdio.h>

static bool isAscii(char c) {
    return c >= 32 && c <= 126;
}

void hexdump(uint8_t* data, size_t length, size_t W, bool ascii) {
    const int H = length / W;
    for (int y = 0; y<H; y++) {
        printf("%4x: ", y*W);
        for (int x = 0; x<W; x++) {
            if (y*W+x >= length) break;
            printf("%02x ", data[y * W + x]);
        }
        if (ascii) {
            printf("  ");
            for (int x = 0; x<W; x++) {
                if (y*W+x >= length) break;
                char byte = data[y * W + x];
                printf("%c", isAscii(byte) ? byte : '.');
            }
        }
        printf("\n");
    }
}