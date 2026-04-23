#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>

typedef struct {
  /* npc read using isa_regread*/
  word_t gpr[32]; /* Don't use */
  // all the GPRs can only be read by gpr(x)
  vaddr_t pc;
} riscv32_CPU_state;

// decode
typedef struct  {
  word_t inst;
#ifdef CONFIG_FTRACE
  word_t rd;
  word_t rs1;
  word_t rs2;
  word_t imm;
#endif
} riscv32_ISADecodeInfo;

#endif
