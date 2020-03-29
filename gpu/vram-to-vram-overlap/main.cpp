#include <common.h>
#include "font.h"

// Write sizexsize rect to vram
void writeRect(int dstX, int dstY, int size) {
    for (int y = 0; y<size; y++) {
        for (int x = 0; x<size; x++) {
            vramPut(dstX + x, dstY + y, y*2 * 32 + x*2);
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

constexpr int testCases[] = {
    2, 8, 15, 16, 
    16, // x + 1
    16, // y + 1
    16, // x + 1, y + 1
};
template<class T, size_t N>
constexpr size_t count(T (&)[N]) { return N; }

constexpr int TEST_W = 3; // Test x from -3 to 3
constexpr int TEST_H = 1; // Test y from -1 to 1
const int COLS = (TEST_W*2 + 1) * (TEST_H*2 + 1);
const int CELL_SIZE = 42;
const int MARGIN = 4;

void drawDebugInfo() {
    // Draw grid, vertical lines
    for (int size = 0; size < count(testCases) + 1; size++) {
        line(
            0, size * CELL_SIZE, 
            CELL_SIZE * (COLS+1), size * CELL_SIZE
        );
    }
    // Horizontal
    for (int i = 0; i <= COLS + 1; i++) {
        line(
            i * CELL_SIZE, 0, 
            i * CELL_SIZE, count(testCases) * CELL_SIZE
        );
    }

    // Labels
    for (int t = 0; t < count(testCases); t++) {
        FntPos(2, t * CELL_SIZE + 12);
        FntPrintf("BLOCK\n  %2d\n ", testCases[t]);
        if (t == 4) FntPrintf("x+1");
        if (t == 5) FntPrintf("y+1");
        if (t == 6) FntPrintf("xy+1");

        int i = 1;
        for (int y = -TEST_H; y <= TEST_H; y++) {
            for (int x = -TEST_W; x <= TEST_W; x++) {
                FntPos(i * CELL_SIZE + 4, (t + 1) * CELL_SIZE - 10);
                FntPrintf("%2d:%d", x, y);
                i++;
            }
        }
    }
}

#define DRAW_DEBUG

int main() {
    initVideo(640, 480);
    printf("\ngpu/vram-to-vram-overlap\n");

    setMaskBitSetting(false, false);
    clearScreen();

#ifdef DRAW_DEBUG
    FntInit();
    drawDebugInfo();
#endif
        
    // Draw test data
    for (int t = 0; t < count(testCases); t++) {
        for (int i = 0; i<COLS; i++) {
            writeRect((i+1) * CELL_SIZE + MARGIN, t * CELL_SIZE + MARGIN, testCases[t]);
            writeGP0(1, 0);
        }
    }
        
    // Run test itself - copy data in different configurations
    for (int t = 0; t < count(testCases); t++) {
        int size = testCases[t];
        int xOffset = 0;
        int yOffset = 0;

        if (t == 4) {xOffset += 1;}
        if (t == 5) {yOffset += 1;}
        if (t == 6) {xOffset += 1; yOffset += 1;}

        int i = 1;
        for (int y = -TEST_H; y <= TEST_H; y++) {
            for (int x = -TEST_W; x <= TEST_W; x++) {
                int srcX = xOffset + i * CELL_SIZE + MARGIN;
                int srcY = yOffset + t * CELL_SIZE + MARGIN;
                int dstX = xOffset + i * CELL_SIZE + MARGIN + x;
                int dstY = yOffset + t * CELL_SIZE + MARGIN + y;
                
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
