#ifndef __ISA_RISCV64_H__
#define __ISA_RISCV64_H__

#include <common.h>
#define RISCV_GPR_NUM 32
typedef struct {
  word_t gpr[32];
  vaddr_t pc;
} riscv64_CPU_state;

typedef struct {
  uint32_t inst;
#ifdef CONFIG_FTRACE
  word_t rd;
  word_t rs1;
  word_t rs2;
  word_t imm;
#endif
} riscv64_ISADecodeInfo;

#endif
