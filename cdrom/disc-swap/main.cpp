#include <common.h>
#include <dma.hpp>
#include <io.h>
#include <psxcd.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>

// Basic test to determine behavior of CD-ROM controller when the lid is opened.

void wait_for_disc() {
  unsigned char last_result = 0xFF;
  bool first = true;
  for (;;)
  {
    unsigned char result[4] = {};
    if (!CdControl(CdlNop, nullptr, result))
      continue;

    if (result[0] != last_result || first) {
      printf("> Getstat: 0x%02X\n", result[0]);
      last_result = result[0];
      first = false;
    }

    if (!(last_result & 0x10))
      break;
  }
}

int main() {
  initVideo(320, 240);
  printf("\ncdrom/disc-swap\n");
  clearScreen();

  for (;;) {
    printf("> Init CD\n");
    CdInit();

    unsigned char status;
    int irq = CdSync(1, &status);
    printf("> CD IRQ=%d, status=0x%02X\n", irq, status);

    if (status & 0x10) {
      printf("> Waiting for disc to be inserted...\n");
      wait_for_disc();
    }

    printf("*** Open the shell now ***\n");

    int last_irq = irq;
    while (last_irq == irq)
      irq = CdSync(1, &status);
    printf("> Got IRQ %d (expected 5), status 0x%02X\n", irq, status);

    printf("*** Close the shell now ***\n");
    wait_for_disc();
    printf("> Disc detected\n");
  }

  return 0;
}
