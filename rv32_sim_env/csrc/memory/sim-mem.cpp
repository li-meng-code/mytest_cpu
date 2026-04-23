#include <common.h>
#include <memory/vaddr.h>
#include <memory/paddr.h>
#include <svdpi.h>

extern "C" word_t sim_mem_read(vaddr_t addr, int len) {
  /* only supports one word read */
  addr &= ~3;
  return vaddr_read(addr, len);
}

extern "C" void sim_mem_write(vaddr_t addr, unsigned int wmask, word_t data) {
  addr &= ~3;
  word_t old_data = vaddr_read(addr, 4);
  word_t new_data = (old_data & ~wmask) | (data & wmask);
  vaddr_write(addr, 4, new_data);
}

#ifdef IS_YSYX_SOC

extern "C" word_t psram_read(vaddr_t addr) {
  return sim_mem_read(addr + 0x80000000L, 4); /* always read one word */
}

extern "C" void psram_write(vaddr_t addr,  word_t data) {
  int shamt = (addr & 0b11) * 8; /* always write one byte */
  unsigned int wmask = 0x000000ffL << shamt;
  data = data << shamt;
  sim_mem_write(addr + 0x80000000L, wmask, data);
}

/* ILLEGAL */
extern "C" word_t vmem_read(vaddr_t addr) {
  vaddr_t base_addr = 0x21000000L;
  return (sim_mem_read(addr + base_addr, 4));
}

extern "C" void vmem_write(vaddr_t addr, unsigned int wmask, word_t data) {
  /* CANNOT USE SIM_MEM_WRITE HERE SINCE IT'S WRITE AFTER READ */
  addr &= ~3;
  data &= wmask;
  vaddr_write(addr, 4, data);
}

extern "C" word_t sdram_read(vaddr_t addr, int id) {
  /* black magic for bitwise and wordwise extension */
  vaddr_t base_addr = ((id&0b10)==0) ? 0xa0000000L : 0xa4000000L;
  return (sim_mem_read(addr + base_addr, 4) >> ((id&0b01) << 4)) & 0xffff;
}

extern "C" void sdram_write(vaddr_t addr, unsigned int wmask, word_t data, int id) {
  /* write 16 bits at most */
  /* black magic for bitwise and wordwise extension */
  vaddr_t base_addr = ((id&0b10)==0) ? 0xa0000000L : 0xa4000000L;
  wmask = (wmask & 0xffff) << ((id&0b01) << 4);
  data  = (data  & 0xffff) << ((id&0b01) << 4);
  sim_mem_write(addr + base_addr, wmask, data);
}

extern "C" void flash_read(int32_t addr, int32_t *data) {
  addr += CONFIG_FLASH_BASE; /* transform to flash internal space */
  *data = paddr_read(addr&~3, 4);
}

extern "C" void mrom_read(int32_t addr, int32_t *data) { 
  // *data = 0x00100073; /* ebreak */ 
  *data = paddr_read(addr&~3, 4);
}

typedef enum { MEM_NONE = 0, MEM_R = 1, MEM_W = 2, MEM_RW = 3 } mem_perm_t;
typedef struct { paddr_t start; paddr_t end; mem_perm_t perm; } mem_region_t;

static const mem_region_t mem_regions[] = {
  {0x02000000, 0x0200FFFF, MEM_R},  // clint
  {0x20000000, 0x2000FFFF, MEM_R},  // mrom
  {0x0F000000, 0x0F01FFFF, MEM_RW}, // sram
  {0x30000000, 0x3FFFFFFF, MEM_R},  // flash
  {0x10000000, 0x10000FFF, MEM_RW}, // uart
  {0x10001000, 0x10001FFF, MEM_RW}, // spi
  {0x80000000, 0x9FFFFFFF, MEM_RW}, // psram
  {0xA0000000, 0xBFFFFFFF, MEM_RW}, // sdram
  {0x21000000, 0x211FFFFF, MEM_W},  // vmem 
  {0x10002000, 0x1000200F, MEM_RW}, // gpio
  {0x10011000, 0x10011007, MEM_R},  // ps2
  {0xC0000000, 0xFFFFFFFF, MEM_RW},  // chipLink 
};

static const int NUM_MEM_REGIONS = sizeof(mem_regions)/sizeof(mem_regions[0]);

static inline int addr_in_region(paddr_t addr, int is_write) {
  for (int i = 0; i < NUM_MEM_REGIONS; i++) {
    if (addr >= mem_regions[i].start && addr <= mem_regions[i].end) {
      if (is_write)
        return mem_regions[i].perm & MEM_W;
      else
        return mem_regions[i].perm & MEM_R;
    }
  }
  return 0;
}

extern "C" void sim_mem_access_check(paddr_t addr, int is_write, paddr_t pc, word_t inst) {
  if (!addr_in_region(addr, is_write)) {
    if (is_write)
      Assert(0, "Illegal write access of address '" FMT_PADDR "' @" FMT_PADDR " inst=" FMT_WORD, addr, pc, inst);
    else
      Assert(0, "Illegal read access of address '" FMT_PADDR "' @" FMT_PADDR " inst=" FMT_WORD, addr, pc, inst);
  }
}

#endif