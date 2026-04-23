#include <common.h>
#include <isa.h>
#include <memory/paddr.h>

#ifdef IS_YSYX_SOC
static const uint32_t img[] = {
  0x00000297,
  0x0102c503,
  0xf1150513,
  0x00100073,
  0xdeadbeef,
};
#else
static const uint32_t img[] = {
  0x00000297,
  0x01400313,
  0x00000393,
  0x00138393,
  0xfe639ce3,
  0x00000513,
  0x00100073,
  0xdeadbeef,
};
#endif

static void init_builtin_img(void) {
  memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));
}

void init_isa() {
  init_builtin_img();
  cpu.pc = RESET_VECTOR;
}
