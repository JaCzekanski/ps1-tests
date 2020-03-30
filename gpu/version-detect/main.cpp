#include <common.h>
#include <stdint.h>

int main() {
    initVideo(320, 240);
    printf("gpu/version-detect\n");

    // GPU version detection based on nocash documentation.
    volatile uint32_t* GP0 = (volatile uint32_t*)0x1F801810;
    volatile uint32_t* GP1 = (volatile uint32_t*)0x1F801814;
    volatile uint32_t* GPUREAD = (volatile uint32_t*)0x1F801810;
    volatile uint32_t* GPUSTAT = (volatile uint32_t*)0x1F801814;

    *GP1 = 0x10000004;
    *GP1 = 0x10000007;

    uint32_t res1 = *GPUREAD;
    printf("> GPUREAD = 0x%08X\n", res1);

    if ((res1 & 0x00FFFFFF) == 2) {
      printf("* GPU version 2 [New 208pin GPU (LATE-PU-8 and up)]\n");
    } else {
      *GP0 = (*GPUSTAT & 0x3FFF) | 0xE1001000;
      uint32_t dummy = *GPUREAD;
      uint32_t res2 = *GPUSTAT;
      printf("> dummy=0x%08X, res2=0x%08X\n", dummy, res2);
      if (res2 & 0x00001000)
        printf("* GPU version 1 [unknown GPU type, maybe some custom arcade/prototype version ?]\n");
      else
        printf("* GPU version 0 [Old 160pin GPU (EARLY-PU-8)]\n");
    }

    for (;;) {
        VSync(0);
    }
    return 0;
}
