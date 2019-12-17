#include <common.h>
#include "font.h"

// 24 bit to 15 bit value
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t c = 0;
    c |= ((r >> 3) & 0x1f);
    c |= ((g >> 3) & 0x1f) << 5;
    c |= ((b >> 3) & 0x1f) << 10;
    return c;
}

constexpr uint16_t randomColor(int c) {
    rgb(64+(c*432)%192, 64+(c*127)%192, 64+(c*941)%192);
}

// Write sizexsize rect to vram
void writeRect(int dstX, int dstY, int size) {
    for (int y = 0; y<size; y++) {
        for (int x = 0; x<size; x++) {
            vramPut(dstX + x, dstY + y, randomColor(y * 32 + x));
        }   
    }
}

void line(int sx, int sy, int ex, int ey) {
    LINE_F2 l;
    setLineF2(&l);
    setRGB0(&l, 255, 255, 255);

    l.x0 = sx;
    l.y0 = sy;
    l.x1 = ex;
    l.y1 = ey;

    DrawPrim(&l);
}

constexpr int SIZES = 6 + 3;
constexpr int sizes[SIZES] = {
    2, 8, 15, 16, 31, 32, 
    32, // x + 1
    32, // y + 1
    32, // x + 1, y + 1
};

const int COLS = 10;
const int CELL_SIZE = 48;
const int MARGIN = 4;

void drawDebugInfo() {
    // Draw grid, vertical lines
    for (int size = 0; size < SIZES + 1; size++) {
        line(
            0, size * CELL_SIZE, 
            CELL_SIZE * COLS, size * CELL_SIZE
        );
    }
    // Horizontal
    for (int i = 0; i<COLS + 1; i++) {
        line(
            i * CELL_SIZE, 0, 
            i * CELL_SIZE, SIZES * CELL_SIZE
        );
    }

    // Labels
    for (int testCase = 0; testCase < SIZES; testCase++) {
        for (int i = 0; i<COLS; i++) {
            if (i == 0) {
                FntPos(i * CELL_SIZE + 4, testCase * CELL_SIZE + 12);
                FntPrintf("BLOCK\n\n  %2d\n ", sizes[testCase]);
                if (testCase == 6) FntPrintf("x+1");
                if (testCase == 7) FntPrintf("y+1");
                if (testCase == 8) FntPrintf("xy+1");
            } else {
                int x = (i-1)%3 - 1;
                int y = (i-1)/3 - 1;
                FntPos(i * CELL_SIZE + 4, (testCase + 1) * CELL_SIZE - 10);
                FntPrintf("%2d:%d", x, y);
            }
        }
    }
}

#define DRAW_DEBUG

int main() {
    initVideo(320, 240);
    printf("\ngpu/vram-to-vram-overlap\n");

    setMaskBitSetting(false, false);
    clearScreen();

#ifdef DRAW_DEBUG
    FntInit();
    drawDebugInfo();
#endif
        
    // Draw test data
    for (int testCase = 0; testCase < SIZES; testCase++) {
        for (int i = 1; i<COLS; i++) {
            writeRect(i * CELL_SIZE + MARGIN, testCase * CELL_SIZE + MARGIN, sizes[testCase]);
            writeGP0(1, 0);
        }
    }
        
    // Run test itself - copy data in different configurations
    for (int testCase = 0; testCase < SIZES; testCase++) {
        int size = sizes[testCase];
        int xOffset = 0;
        int yOffset = 0;

        if (testCase == 6) {xOffset += 1;}
        if (testCase == 7) {yOffset += 1;}
        if (testCase == 8) {xOffset += 1; yOffset += 1;}

        int i = 1;
        for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
                int srcX = xOffset + i        * CELL_SIZE + MARGIN;
                int srcY = yOffset + testCase * CELL_SIZE + MARGIN;
                int dstX = xOffset + i        * CELL_SIZE + MARGIN + x;
                int dstY = yOffset + testCase * CELL_SIZE + MARGIN + y;
                
                vramToVramCopy(srcX, srcY, dstX, dstY, size, size);
                writeGP0(1, 0);
                i++;
            }   
        }
    }
    
    for (;;) {
        VSync(0);
    }
    return 0;
}
