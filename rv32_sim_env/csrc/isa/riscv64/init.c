#include <common.h>
#include <memory/paddr.h>
#include <isa.h>

#ifdef IS_YSYX_SOC
static const uint32_t img[5] = {
  0x00000297,  // auipc t0,0
  0x0102c503,  // lbu a0,16(t0)
  0xf1150513,  // addi a0, -0xef /* you can't write it because flash is read-only */
  0x00100073,  // ebreak
  0xdeadbeef,  // some data
};
#else
static const uint32_t img[] = {
  // 0x00000297,  // 0x80000000: auipc t0(x5), 0          t0 = 0x80000000
  0x01400313,  // 0x80000004: addi  t1(x6), x0, 20     loop bound = 20
  0x00000393,  // 0x80000008: addi  t2(x7), x0, 0      i = 0
  // 0x0202c503,  // 0x8000000c: lbu   a0(x10), 32(t0)    load *(0x80000020)
  0x00138393,  // 0x80000010: addi  t2, t2, 1          i++
  // 0xfe639ce3,  // 0x80000014: bne   t2, t1, -8         loop
  // 0x00000513,  // 0x80000018: addi  a0, x0, 0          <<< 显式清零 a0
  0x00100073,  // 0x8000001c: ebreak                   trap here (a0 == 0)
  0xdeadbeef,  // 0x80000020: some data               保持不变
};

#endif

void init_builtin_img() {
  memcpy(guest_to_host(RESET_VECTOR), img, sizeof(img));
}

void init_isa() {
  init_builtin_img();
  cpu.pc = RESET_VECTOR;
}
