#ifndef __MEMORY_HOST_H__
#define __MEMORY_HOST_H__

#include <common.h>

static inline word_t host_read(void *addr, int len) {
  switch (len) {
    case 1: return *(uint8_t  *)addr;
    case 2: return *(uint16_t *)addr;
    case 4: return *(uint32_t *)addr;
      default: Assert(0, "cannot read %d bytes", len);
  }
}

static inline void host_write(void *addr, int len, word_t data) {
  switch (len) {
    case 1: *(uint8_t  *)addr = (uint8_t )data; return;
    case 2: *(uint16_t *)addr = (uint16_t)data; return;
    case 4: *(uint32_t *)addr = (uint32_t)data; return;
    default: Assert(0, "cannot write %d bytes", len);
  }
}

#endif

