#include <common.h>
#include <dma.hpp>
#include <io.h>
#include <psxcd.h>
#include <psxpad.h>
#include <psxapi.h>
#include <psxetc.h>
#include <stdint.h>

// BCD helpers
constexpr uint8_t BinaryToPackedBCD(uint8_t value)
{
  return ((value / 10) << 4) + (value % 10);
}
constexpr uint8_t PackedBCDToBinary(uint8_t value)
{
  return ((value >> 4) * 10) + (value % 16);
}
constexpr uint8_t IsValidBCDDigit(uint8_t digit)
{
  return (digit <= 9);
}
constexpr uint8_t IsValidPackedBCD(uint8_t value)
{
  return IsValidBCDDigit(value & 0x0F) && IsValidBCDDigit(value >> 4);
}

enum StatBits : uint8_t
{
	STAT_ERROR = (1 << 0),
	STAT_MOTOR_ON = (1 << 1),
	STAT_SEEK_ERROR = (1 << 2),
	STAT_ID_ERROR = (1 << 3),
	STAT_SHELL_OPEN = (1 << 4),
	STAT_READING = (1 << 5),
	STAT_SEEKING = (1 << 6),
	STAT_PLAYING_CDDA = (1 << 7)
};

static void wait_for_disc() {
  printf("> Initializing CD-ROM controller...\n");
  CdInit();

  uint8_t result[4];
  CdControl(CdlNop, nullptr, result);

  if (!(result[0] & STAT_SHELL_OPEN)) {
    // disc inserted, we already initialzied
    return;
  }

  printf("* Please insert a disc and close the lid\n");
  do {
    CdControl(CdlNop, nullptr, result);
  } while (result[0] & STAT_SHELL_OPEN);

  printf("> Disc inserted\n");
  CdInit();
}

static bool cd_setloc(int m, int s, int f) {
	uint8_t param[3] = { BinaryToPackedBCD(m), BinaryToPackedBCD(s), BinaryToPackedBCD(f) };
	CdControl(CdlSetloc, param, nullptr);

  uint8_t status;
  int irq = CdSync(0, &status);
  if (irq != CdlComplete) {
    printf("* SetLoc failed, irq=%d, status=0x%02X\n", irq, status);
    return false;
  }

  return true;
}

static bool cd_seek(int m, int s, int f, bool logical = true, bool sync = true) {
	uint8_t param[3] = { BinaryToPackedBCD(m), BinaryToPackedBCD(s), BinaryToPackedBCD(f) };

	printf("> Seeking to [%02d:%02d:%02d] (%s)\n", m, s, f, logical ? "logical" : "physical");
  if (!cd_setloc(m, s, f))
    return false;

  CdControl(CdlSeekL, param, nullptr);
  if (!sync)
    return true;

  uint8_t status;
  int irq = CdSync(0, &status);
  printf("> Seek result irq=%d, status=0x%02X\n", irq, status);

  if (irq != CdlComplete) {
    printf("* Seek failed, irq=%d, status=0x%02X\n", irq, status);
    return false;
  }

  return true;
}

static bool test_stat_bits(uint8_t expected_bits) {
  uint8_t result[4] = {};
  CdControl(CdlNop, nullptr, result);

  printf("> GetStat -> 0x%02X\n", result[0]);
  if (result[0] != expected_bits) {
    printf("! Expected stat bits 0x%02X, got 0x%02X\n", expected_bits, result[0]);
    return false;
  }

  return true;
}

static bool test_GetlocL(bool expected_success) {
  constexpr size_t header_size = 4 /* cd header */ + 4 /* xa header */;
  uint8_t result[header_size] = {};
  CdControl(CdlGetlocL, nullptr, result);

  uint8_t status = 0;
  int irq = CdSync(0, &status);
  if (irq != CdlComplete) {
    printf("> GetlocL failed, IRQ = %d, status = 0x%02X\n", irq, status);
    if (expected_success) {
      printf("! Expected success but GetlocL failed\n");
      return false;
    }
    return true;
  }

  printf("> GetlocL succeeded - [%02x:%02x:%02x] mode %d\n", result[0], result[1], result[2], result[3]);
  if (!expected_success) {
    printf("! Expected failure but GetlocL succeeeded\n");
    return false;
  }

  return true;
}

static bool test_GetlocP(bool expected_success) {
  constexpr size_t header_size = 8;
  uint8_t result[header_size] = {};
  CdControl(CdlGetlocP, nullptr, result);

  uint8_t status = 0;
  int irq = CdSync(0, &status);
  if (irq != CdlComplete) {
    printf("> GetlocP failed, IRQ = %d, status = 0x%02X\n", irq, status);
    if (expected_success) {
      printf("! Expected success but GetlocP failed\n");
      return false;
    }
    return true;
  }

  printf("> GetlocP succeeded - track %02x index %02x [%02x:%02x:%02x] absolute [%02x:%02x:%02x]\n", result[0], result[1], result[2], result[3], result[4], result[5], result[6], result[7]);
  if (!expected_success) {
    printf("! Expected failure but GetlocP succeeeded\n");
    return false;
  }

  return true;
}
static bool do_test() {
  static uint8_t sector_buffer[2352 - 12];
  bool result = true;

  wait_for_disc();

  // header valid should be clear after init/reset
  printf("> Testing state of reading after initial reset\n");
  result &= test_stat_bits(STAT_MOTOR_ON);

  // GetlocL should fail
  printf("> Testing Getloc after initial reset\n");
  result &= test_GetlocL(false);
  result &= test_GetlocP(true);

  // seek to something a bit further away
  printf("> Testing seek...\n");
  if (!cd_seek(0, 2, 16, true, true)) {
    result = false;
  }

  // we don't see the seeking bit go active because the command is synchronous
  result &= test_stat_bits(STAT_MOTOR_ON);

  // GetlocL should succeed
  printf("> Testing Getloc after seek\n");
  result &= test_GetlocL(true);
  result &= test_GetlocP(true);

  // start reading a single sector - this should require a seek since the disc is still moving
  cd_setloc(40, 0, 16);
  printf("> Testing read\n");
  CdRead(1, reinterpret_cast<unsigned int*>(sector_buffer), CdlModeSize1);

  // delay a bit to give it time to start seeking
  for (int i = 0; i < 1000000; i++)
    asm volatile ("nop\n");

  result &= test_stat_bits(STAT_SEEKING | STAT_MOTOR_ON);

  // wait for the read to complete, this will auto-send pause
  printf("> Waiting for read to complete\n");
  if (CdReadSync(0, nullptr)) {
    printf("! CD read failed\n");
    result = false;
  }

  // reading bit should be clear
  result &= test_stat_bits(STAT_MOTOR_ON);

  // GetLocL should succeed
  printf("> Testing GetLoc after reading\n");
  result &= test_GetlocL(true);
  result &= test_GetlocP(true);

  // reset the CDC - header valid should be clear
  printf("> Testing bits after reset\n");
  CdInit();
  result &= test_stat_bits(STAT_MOTOR_ON);

  // getlocl should succeed - it only fails on first power-on
  printf("> Testing Getloc after reset\n");
  result &= test_GetlocL(true);
  result &= test_GetlocP(true);

  // seek to start - should still succeed
  printf("> Testing Getloc after seek to start\n");
  cd_seek(0, 0, 30);
  result &= test_GetlocL(true);
  result &= test_GetlocP(true);

  // seek to lead-out - should still succeed
  printf("> Testing Getloc after seek to end\n");
  cd_seek(74, 0, 0);
  result &= test_GetlocL(true);
  result &= test_GetlocP(true);

  // seek out of range - should fail
  cd_seek(74, 30, 0);
  result &= test_GetlocL(false);
  result &= test_GetlocP(false);

  return result;
}


int main() {
  initVideo(320, 240);
  printf("\ncdrom/header-valid-bit\n");
  clearScreen();

  const bool result = do_test();
  printf("\nTest %s\n", result ? "passed" : "failed");

  for (;;) {
    VSync(0);
  }

  return 0;
}
