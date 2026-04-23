#ifndef __MEMORY__PADDR_H__
#define __MEMORY__PADDR_H__

#include <common.h>
#include <memory/host.h>

/* the memory space mapped is bigger or equivalent to the actual storage size */
/* default pmem (psram) space: 0x8000_0000~0x80ff_ffff */
#define PMEM_LEFT  ((paddr_t)CONFIG_MBASE)
#define PMEM_RIGHT ((paddr_t)CONFIG_MBASE + CONFIG_MSIZE - 1)
/* mrom space: 0x2000_0000~0x2000_0fff */
#define MROM_LEFT  ((paddr_t)CONFIG_MROM_BASE)
#define MROM_RIGHT ((paddr_t)CONFIG_MROM_BASE + CONFIG_MROM_SIZE - 1)
/* flash space: 0x3000_0000~0x3fff_ffff */
#define FLASH_LEFT ((paddr_t)CONFIG_FLASH_BASE)
#define FLASH_RIGHT ((paddr_t)CONFIG_FLASH_BASE + CONFIG_FLASH_SIZE - 1)
/* sdram space: 0xa000_0000~0xbfff_ffff */
#define SDRAM_LEFT ((paddr_t)CONFIG_SDRAM_BASE)
#define SDRAM_RIGHT ((paddr_t)CONFIG_SDRAM_BASE + CONFIG_SDRAM_SIZE - 1)
/* vmem space: 0x2100_0000~0x211f_ffff */
#define VMEM_LEFT ((paddr_t)CONFIG_VMEM_BASE)
#define VMEM_RIGHT ((paddr_t)CONFIG_VMEM_BASE + CONFIG_VMEM_SIZE - 1)

#ifdef IS_YSYX_SOC
// #define RESET_VECTOR (MROM_LEFT + CONFIG_PC_RESET_OFFSET) 
#define RESET_VECTOR (FLASH_LEFT + CONFIG_PC_RESET_OFFSET)
#else
#define RESET_VECTOR (PMEM_LEFT + CONFIG_PC_RESET_OFFSET)
#endif

static inline bool in_pmem(paddr_t addr) {
  return (addr - CONFIG_MBASE) < CONFIG_MSIZE;
}

static inline bool in_mrom(paddr_t addr) { /* mrom is read-only */
  return (addr - CONFIG_MROM_BASE) < CONFIG_MROM_SIZE;
}

static inline bool in_flash(paddr_t addr) { /* flash is read-only */
  return (addr - CONFIG_FLASH_BASE) < CONFIG_FLASH_SIZE;
}

static inline bool in_sdram(paddr_t addr) {
  return (addr - CONFIG_SDRAM_BASE) < CONFIG_SDRAM_SIZE;
}

static inline bool in_vmem(paddr_t addr) {
  return (addr - CONFIG_VMEM_BASE) < CONFIG_VMEM_SIZE;
}

static inline bool in_storage(paddr_t addr) {
  if (!(in_pmem(addr) || in_mrom(addr) || in_flash(addr) || in_sdram(addr) || in_vmem(addr))) {
    printf( 
      "The address '" FMT_PADDR "' is not in any of the available storages:\n"
      "psram: [" FMT_PADDR " : " FMT_PADDR "]\n"
      "mrom:  [" FMT_PADDR " : " FMT_PADDR "] (read only)\n"
      "flash: [" FMT_PADDR " : " FMT_PADDR "]\n"
      "sdram: [" FMT_PADDR " : " FMT_PADDR "]\n"
      "vmem:  [" FMT_PADDR " : " FMT_PADDR "]\n",
      addr,
      PMEM_LEFT, PMEM_RIGHT,
      MROM_LEFT, MROM_RIGHT,
      FLASH_LEFT, FLASH_RIGHT,
      SDRAM_LEFT, SDRAM_RIGHT,
      VMEM_LEFT, VMEM_RIGHT
    );
    return false;
  }
  return true;
}

uint8_t *guest_to_host(paddr_t paddr);
paddr_t host_to_guest(uint8_t *haddr);

word_t paddr_read(paddr_t addr, int len);
void paddr_write(paddr_t addr, int len, word_t data);

#endif