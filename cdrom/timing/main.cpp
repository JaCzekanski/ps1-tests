#include <common.h>
#include <io.h>
#include <malloc.h>
#include <measure.hpp>
#include <psxapi.h>
#include <psxcd.h>
#include <psxetc.h>
#include <stdint.h>

// BCD helpers
constexpr uint8_t BinaryToPackedBCD(uint8_t value) {
  return ((value / 10) << 4) + (value % 10);
}
constexpr uint8_t PackedBCDToBinary(uint8_t value) {
  return ((value >> 4) * 10) + (value % 16);
}
constexpr uint8_t IsValidBCDDigit(uint8_t digit) { return (digit <= 9); }
constexpr uint8_t IsValidPackedBCD(uint8_t value) {
  return IsValidBCDDigit(value & 0x0F) && IsValidBCDDigit(value >> 4);
}

enum StatBits : uint8_t {
  STAT_ERROR = (1 << 0),
  STAT_MOTOR_ON = (1 << 1),
  STAT_SEEK_ERROR = (1 << 2),
  STAT_ID_ERROR = (1 << 3),
  STAT_SHELL_OPEN = (1 << 4),
  STAT_READING = (1 << 5),
  STAT_SEEKING = (1 << 6),
  STAT_PLAYING_CDDA = (1 << 7)
};

volatile uint8_t *CD_REG0 = (volatile uint8_t *)0x1F801800;
volatile uint8_t *CD_REG1 = (volatile uint8_t *)0x1F801801;
volatile uint8_t *CD_REG2 = (volatile uint8_t *)0x1F801802;
volatile uint8_t *CD_REG3 = (volatile uint8_t *)0x1F801803;

static bool has_cd_response() { return (*CD_REG0 & (1u << 5)) != 0; }

static void write_cd_index(uint8_t index) { *CD_REG0 = index & 3; }

static uint8_t read_cd_irq_flag() {
  write_cd_index(1);
  return *CD_REG3;
}

static uint8_t read_cd_irq_enable() {
  write_cd_index(0);
  return *CD_REG3;
}

static uint8_t read_cd_response() { return *CD_REG1; }

static void write_cd_reg(uint8_t index, uint8_t value) {
  uint8_t r_offset = (index % 3);
  uint8_t r_index = (index / 3);
  write_cd_index(r_index);
  switch (r_offset) {
  case 0:
    *CD_REG1 = value;
    break;
  case 1:
    *CD_REG2 = value;
    break;
  case 2:
    *CD_REG3 = value;
    break;
  }
}

static void write_cd_irq_enable(uint8_t value) { write_cd_reg(4, value); }
static void write_cd_irq_flag(uint8_t value) { write_cd_reg(5, value); }
static void write_cd_parameter(uint8_t value) { write_cd_reg(1, value); }
static void write_cd_command(uint8_t value) { write_cd_reg(0, value); }

static bool wait_for_cd_irq(uint8_t want_irq, bool clear) {
  for (;;) {
    uint8_t irq = read_cd_irq_flag() & 0x0F;
    if (irq == 0)
      continue;

    if (irq == 5) {
      printf("Unexpected INT5!");
      while (has_cd_response()) {
        printf(" 0x%02X", read_cd_response());
      }
      printf("\n");
      write_cd_irq_flag(0x1F);
      continue;
    }

    if (irq != want_irq) {
      write_cd_irq_flag(0x1F);
      continue;
    }

    if (clear)
      write_cd_irq_flag(0x1F);
    return true;
  }
}

static void wait_for_cd_stat_bits_clear(uint8_t bits) {
  uint8_t old_enable = read_cd_irq_enable();
  write_cd_irq_enable(0x00);
  for (;;) {
    write_cd_command(CdlNop);
    if (!wait_for_cd_irq(3, true)) {
      printf("Getstat failed\n");
      return;
    }
    uint8_t res = read_cd_response();
    while (has_cd_response())
      read_cd_response();
    if ((res & bits) == 0x00)
      break;
  }
  write_cd_irq_enable(old_enable);
}

static void write_cd_parameters(const uint8_t *params, uint32_t plen) {
  for (uint32_t i = 0; i < plen; i++)
    write_cd_parameter(params[i]);
}

static void set_cd_mode(uint8_t mode) {
  uint8_t old_enable = read_cd_irq_enable();
  write_cd_irq_enable(0x00);
  write_cd_parameter(mode);
  write_cd_command(CdlSetmode);
  if (!wait_for_cd_irq(3, true)) {
    printf("Setmode failed\n");
  }
  write_cd_irq_flag(0x40);
  write_cd_irq_enable(old_enable);
}

static uint8_t s_command_to_execute;

static uint32_t measure_ack_time(uint8_t command, const uint8_t *params,
                                 uint32_t plen) {
  uint8_t old_enable = read_cd_irq_enable();
  write_cd_irq_enable(0);
  write_cd_irq_flag(0x1F);

  if (plen > 0)
    write_cd_parameters(params, plen);

  s_command_to_execute = command;
  return measure([] {
    write_cd_command(s_command_to_execute);
    wait_for_cd_irq(3, true);
  });

  write_cd_irq_enable(old_enable);
}

static void test_ack_time(const char *test_name, uint8_t command,
                          const uint8_t *params, uint32_t plen,
                          uint32_t count = 1000) {
  uint32_t total_time = 0;
  uint32_t min_time = 0xFFFFFFFFu;
  uint32_t max_time = 0;

  for (uint32_t i = 0; i < count; i++) {
    uint32_t time = measure_ack_time(command, params, plen);
    min_time = (time < min_time) ? time : min_time;
    max_time = (time > max_time) ? time : max_time;
    total_time += time;
  }

  printf("%s: min %u, max %u, average %u over %u runs\n", test_name, min_time,
         max_time, total_time / count, count);
}

static void test_init(uint32_t count = 1) {
  uint32_t total_ack_time = 0;
  uint32_t min_ack_time = 0xFFFFFFFFu;
  uint32_t max_ack_time = 0;
  uint32_t total_complete_time = 0;
  uint32_t min_complete_time = 0xFFFFFFFFu;
  uint32_t max_complete_time = 0;

  uint8_t old_enable = read_cd_irq_enable();
  write_cd_irq_enable(0);

  for (uint32_t i = 0; i < count; i++) {
    uint32_t ack_time = measure_ack_time(CdlInit, nullptr, 0);
    uint32_t complete_time = measure([]() { wait_for_cd_irq(2, true); });

    total_ack_time += ack_time;
    min_ack_time = (ack_time < min_ack_time) ? ack_time : min_ack_time;
    max_ack_time = (ack_time > max_ack_time) ? ack_time : max_ack_time;

    total_complete_time += complete_time;
    min_complete_time =
        (complete_time < min_complete_time) ? complete_time : min_complete_time;
    max_complete_time =
        (complete_time > max_complete_time) ? complete_time : max_complete_time;
  }

  write_cd_irq_enable(old_enable);

  printf("init ack: min %u, max %u, average %u\n", min_ack_time, max_ack_time,
         total_ack_time / count);
  printf("init complete: min %u, max %u, average %u\n", min_complete_time,
         max_complete_time, total_complete_time / count);
}

static uint32_t *s_read_sector_times;
static uint32_t *s_read_sector_times_ptr;

static void test_read(uint8_t m, uint8_t s, uint8_t f, uint32_t num_sectors,
                      bool double_speed) {
  s_read_sector_times = (uint32_t *)malloc(sizeof(uint32_t) * num_sectors);
  s_read_sector_times_ptr = s_read_sector_times;

  set_cd_mode(double_speed ? 0x80 : 0x00);
  uint8_t old_enable = read_cd_irq_enable();
  write_cd_irq_enable(0);
  write_cd_irq_flag(0x1F);

  const uint8_t loc[] = {BinaryToPackedBCD(m), BinaryToPackedBCD(s),
                         BinaryToPackedBCD(f)};
  write_cd_parameters(loc, sizeof(loc) / sizeof(loc[0]));
  const uint32_t setloc_ack_time = measure([] {
    write_cd_command(CdlSetloc);
    wait_for_cd_irq(3, true);
  });

  const uint32_t ack_time = measure([]() {
    write_cd_command(CdlReadN);
    wait_for_cd_irq(3, true);
  });

  // We must wait for the seeking bit to clear, otherwise we get an error.
  // There's a time period inbetween the ACK and before the seeking bit gets set
  // that it's also invalid Instead we just wait for the first sector...
  // wait_for_cd_stat_bits_clear(STAT_SEEKING);
  const uint32_t first_sector_time =
      measure([]() { wait_for_cd_irq(1, true); });

  for (uint32_t i = 0; i < num_sectors; i++) {
    *(s_read_sector_times_ptr++) = measure([]() { wait_for_cd_irq(1, true); });
  }

  const uint32_t pause_ack_time = measure([]() {
    write_cd_command(CdlPause);
    wait_for_cd_irq(3, true);
  });

  const uint32_t pause_complete_time =
      measure([]() { wait_for_cd_irq(2, true); });

  uint32_t min_sector_time = 0xffffffff;
  uint32_t max_sector_time = 0;
  uint32_t avg_sector_time = 0;
  for (uint32_t i = 0; i < num_sectors; i++) {
    const uint32_t t = s_read_sector_times[i];
    min_sector_time = (t < min_sector_time) ? t : min_sector_time;
    max_sector_time = (t > max_sector_time) ? t : max_sector_time;
    avg_sector_time += s_read_sector_times[i];
  }
  avg_sector_time /= num_sectors;

  printf("%s timing: Setloc ACK = %u ticks, ReadN ACK = %u ticks, First Sector "
         "= %u, Remaining Sectors = min %u max %u avg %u ticks, Pause ACK = %u "
         "ticks, Pause Complete = %u ticks\n",
         double_speed ? "double-speed" : "single-speed", setloc_ack_time,
         ack_time, first_sector_time, min_sector_time, max_sector_time,
         avg_sector_time, pause_ack_time, pause_complete_time);

  write_cd_irq_flag(0x1F);
  write_cd_irq_enable(old_enable);

  free(s_read_sector_times);
  s_read_sector_times = nullptr;
  s_read_sector_times_ptr = nullptr;

  set_cd_mode(0x00);
}

int main() {
  initVideo(320, 240);
  printf("\ncdrom/timing\n");
  clearScreen();

  CdInit();

  constexpr uint8_t test_msf[] = {0, 2, 16};
  constexpr uint8_t test_mode[] = {0x00};

  test_init(100);

  test_ack_time("CdlNop", CdlNop, nullptr, 0, 100);
  test_ack_time("CdlMute", CdlMute, nullptr, 0, 100);
  test_ack_time("CdlDemute", CdlDemute, nullptr, 0, 100);
  test_ack_time("CdlSetloc", CdlSetloc, test_msf,
                sizeof(test_msf) / sizeof(test_msf[0]), 100);
  test_ack_time("CdlSetmode", CdlSetmode, test_mode,
                sizeof(test_mode) / sizeof(test_mode[0]), 100);

  // single-speed reads
  for (uint32_t i = 0; i < 5; i++) {
    test_read(0, 2, 16, 100, false);
  }

  // double-speed reads
  for (uint32_t i = 0; i < 5; i++) {
    test_read(0, 2, 16, 100, true);
  }

  for (;;) {
    VSync(0);
  }

  return 0;
}
