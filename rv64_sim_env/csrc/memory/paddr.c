#include <isa.h>
#include <memory/paddr.h>
#include <common.h>
#include <cpu/cpu.h>
#include <generated/autoconf.h>

#define FLASH_LOAD_CHAR_TEST
#ifdef FLASH_LOAD_CHAR_TEST
#include "char-test.h"
#endif

/* mrom space: 0x2000_0000 ~ 0x2000_0fff */
/* sram is not accessed by C: 0x0f000000 ~ 0x0f001fff */
static uint8_t mrom [CONFIG_MROM_SIZE]  PG_ALIGN = {};
static uint8_t pmem [CONFIG_MSIZE]      PG_ALIGN = {};
/* psram */
static uint8_t flash[CONFIG_FLASH_SIZE] PG_ALIGN = {
  0x78, 0x56, 0x34, 0x12,
  0x12, 0x34, 0x56, 0x78
}; /* magic number to test */
static uint8_t sdram[CONFIG_SDRAM_SIZE] PG_ALIGN = {};
static uint8_t vmem [CONFIG_VMEM_SIZE]  PG_ALIGN = {};

uint8_t *guest_to_host(paddr_t paddr) {
  if (likely(in_pmem(paddr)))  return pmem  + (size_t)(paddr - CONFIG_MBASE);
  if (likely(in_mrom(paddr)))  return mrom  + (size_t)(paddr - CONFIG_MROM_BASE);
  if (likely(in_flash(paddr))) return flash + (size_t)(paddr - CONFIG_FLASH_BASE);
  if (likely(in_sdram(paddr))) return sdram + (size_t)(paddr - CONFIG_SDRAM_BASE);
  if (likely(in_vmem(paddr)))  return vmem  + (size_t)(paddr - CONFIG_VMEM_BASE);
  assert(0);
  return NULL;
}

paddr_t host_to_guest(uint8_t *haddr) {
  if (likely((size_t)(haddr - pmem)  < CONFIG_MSIZE))      return (paddr_t)(haddr - pmem)  + CONFIG_MBASE;
  if (likely((size_t)(haddr - mrom)  < CONFIG_MROM_SIZE))  return (paddr_t)(haddr - mrom)  + CONFIG_MROM_BASE;
  if (likely((size_t)(haddr - flash) < CONFIG_FLASH_SIZE)) return (paddr_t)(haddr - flash) + CONFIG_FLASH_BASE;
  if (likely((size_t)(haddr - sdram) < CONFIG_SDRAM_SIZE)) return (paddr_t)(haddr - sdram) + CONFIG_SDRAM_BASE;
  if (likely((size_t)(haddr - vmem)  < CONFIG_VMEM_SIZE))  return (paddr_t)(haddr - vmem)  + CONFIG_VMEM_BASE;
  assert(0);
  return 0;
}

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}

static void out_of_bound(paddr_t addr, const char *wr) {
  assert(!in_storage(addr)); /* otherwise the execution should not reach here */
  panic("%s address = " FMT_PADDR " is out of the storage system at pc = " FMT_WORD,
        wr, addr, cpu.pc);
}

void init_mem() {
#ifdef IS_YSYX_SOC
  memset(pmem, 0, sizeof(pmem));
#else
  memset(pmem, rand(), sizeof(pmem));
#endif
  memset(mrom, rand(), sizeof(mrom));
#ifdef FLASH_LOAD_CHAR_TEST
  memcpy(flash, char_test, sizeof(char_test));
#endif

  Log("psram area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
  Log("mrom area [" FMT_PADDR ", " FMT_PADDR "]", MROM_LEFT, MROM_RIGHT);
  Log("flash area [" FMT_PADDR ", " FMT_PADDR "]", FLASH_LEFT, FLASH_RIGHT);
  Log("sdram area [" FMT_PADDR ", " FMT_PADDR "]", SDRAM_LEFT, SDRAM_RIGHT);
  Log("vmem area [" FMT_PADDR ", " FMT_PADDR "]", VMEM_LEFT, VMEM_RIGHT);
}

#define CONFIG_DEVICE

word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_storage(addr))) {
    /* 0x8000_0000 - 0x8fff_ffff */
    return pmem_read(addr, len);
  }

#ifdef CONFIG_DEVICE
  word_t mmio_read(paddr_t addr, int len);
  return mmio_read(addr, len);
#endif

  out_of_bound(addr, "read");
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr)) || likely(in_sdram(addr)) || likely(in_vmem(addr))) {
    pmem_write(addr, len, data);
    return;
  }

#ifdef CONFIG_DEVICE
  void mmio_write(paddr_t addr, int len, word_t data);
  mmio_write(addr, len, data);
  return;
#endif

  out_of_bound(addr, "write");
}