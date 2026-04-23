#ifndef __HOST_H__
#define __HOST_H__

#include <common.h>

static inline word_t host_read(void *addr, int len) {
  switch (len) {
    case 1: return *(uint8_t  *)addr;
    case 2: return *(uint16_t *)addr;
    case 4: return *(uint32_t *)addr;
    case 8: return *(uint64_t *)addr;
    default:
      panic("cannot read %d bytes", len);
      assert(0);
      return 0;
  }
}

static inline void host_write(void *addr, int len, word_t data) {
  switch (len) {
    case 1: *(uint8_t  *)addr = (uint8_t )data; break;
    case 2: *(uint16_t *)addr = (uint16_t)data; break;
    case 4: *(uint32_t *)addr = (uint32_t)data; break;
    case 8: *(uint64_t *)addr = (uint64_t)data; break;
    default:
      panic("cannot write %d bytes", len);
      assert(0);
  }
}

#endif