#ifndef __RISCV64_REG_H__
#define __RISCV64_REG_H__

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

word_t gpr(unsigned int i);

static inline unsigned int check_reg_idx(unsigned int idx) {
  Assert(idx < 32, "Illegal register read 'gpr(%u)'", idx);
  return idx;
}

static inline const char *reg_name(int idx) {
  extern const char *regs[];
  return regs[check_reg_idx(idx)];
}

#ifdef __cplusplus
}
#endif

#endif
