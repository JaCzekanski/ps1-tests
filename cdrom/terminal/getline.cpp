#include "getline.h"
#include <psxapi.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BUFFER_LEN 128
#define HISTORY_SIZE 16

static int historySize = 0;
static char history[HISTORY_SIZE][BUFFER_LEN] = {};

bool isAscii(char c) {
    return c >= 32 && c <= 126;
}

bool getline(char* buf, unsigned length) {
    buf[0] = 0;
    int ptr = 0;
    int escapeCode = 0;

    int historyPtr = -1;

    while (true) {
        char c = getchar();

        if (escapeCode == 0 && c == 27) {
            escapeCode = 1;
            continue;
        }
        if (escapeCode == 1) {
            if (c == '[') {
                escapeCode = 2;
            } else {
                escapeCode = 0;
            }
            continue;
        }
        if (escapeCode == 2) {
            auto clearAndPrint = [&](){
                for (int i = 0; i<ptr; i++) putchar('\b');
                for (int i = 0; i<ptr; i++) putchar(' ');
                for (int i = 0; i<ptr; i++) putchar('\b');
                
                if (historyPtr == -1) {
                    strcpy(buf,"");
                } else {
                    strcpy(buf, history[historyPtr]);
                }
                ptr = strlen(buf);

                puts(buf);
            };

            if (c == 'A') { // UP
                if (historyPtr < historySize - 1) {
                    historyPtr++;
                    clearAndPrint();
                }
            } else if (c == 'B')  { // DOWN
                if (historyPtr >= 0) {
                    historyPtr--;
                    clearAndPrint();
                }
            }
            // else if (c == 'C') putchar(6); // right 
            else if (c == 'D') {
                if (ptr > 0) {
                    ptr--;
                    putchar('\b'); // left
                }
            }
            else printf("unknown escape code %d (%c)\n", c, c);
            escapeCode = 0;
            continue;
        }
        
        if (c == 8 || c == 127) {
            if (ptr > 0) {
                buf[--ptr] = 0;
                putchar('\b');
                putchar(' ');
                putchar('\b');
            }
        }
        else if (c == '\r') {
            puts("\r\n");
            if (ptr > 0) {
                if (historySize < HISTORY_SIZE) historySize++;
                for (int i = historySize; i>0; i--) {
                    strcpy(history[i], history[i-1]);
                }
                strcpy(history[0], buf);
            }
            return true;
        }
        else if (c == 3) {
            buf[0] = 0;
            return false; // ctrl+c
        }
        else if (c == 20) {
            printf("\nHISTORY:\n");
            for (int i = 0; i<historySize; i++){
                printf("%d. %s", i, history[i]);
                if (i == historyPtr) printf("  <---");
                printf("\n");
            }
        }
        // 26 - ctrl+z, 24 - ctrl+x, 20 - ctrl+t
        else if (isAscii(c)) {
            if (ptr >= length-1) {
                return true;
            } else {
                putchar(c);
                buf[ptr] = c;
                buf[ptr + 1] = 0;

                ptr++;
            }
        }
        else printf("Unhandled code %d\n", c);
    }

    return true;
}