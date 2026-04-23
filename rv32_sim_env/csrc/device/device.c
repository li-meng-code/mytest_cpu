#include <common.h>
#include <isa.h>

#define RTC_ADDR 0xa0000048
#define SERIAL_ADDR 0xa00003f8

static word_t uptime_h;
static word_t uptime_l;
static void update_uptime() { // virtual uptime
  uint64_t us = get_time();
  uptime_l = us;
  uptime_h = us >> 32;
}

static word_t serial_tx;

#ifdef CONFIG_DIFFTEST
void difftest_skip_ref();
#else
void difftest_skip_ref() {};
#endif

word_t mmio_read(paddr_t addr, int len) {
  difftest_skip_ref();
  if (addr == RTC_ADDR) {
    assert(len == 4);
    word_t data = uptime_l;
    return data;
  }
  else if (addr == RTC_ADDR + 0x4) {
    assert(len == 4);
    word_t data = uptime_h;
    update_uptime();
    return data;
  }
  panic("mmio read not available @ " FMT_PADDR ", pc=" FMT_PADDR "\n", addr, cpu.pc);
}

void mmio_write(paddr_t addr, int len, word_t data) {
  difftest_skip_ref();
  if (addr == SERIAL_ADDR) {
    assert(len == 1);
    putchar((uint8_t)data);
    fflush(stdout);
    return;
  }
  panic("mmio write not available @ " FMT_PADDR ", pc=" FMT_PADDR "\n", addr, cpu.pc);
}