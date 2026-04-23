#include <isa.h>
#include <common.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

static void print_reg(const char *name, word_t reg) {
  printf("%-4s\t" FMT_WORD "\t%-20llu\n", name, reg, (unsigned long long)reg);
}

void isa_reg_display() {
  for (int i = 0; i < 32; ++i) {
    print_reg(regs[i], gpr(i));
  }
  print_reg("pc", cpu.pc);
}

word_t isa_reg_str2val(const char *s, bool *success) {
  if (strcmp("$0", s) == 0) {
    *success = true;
    return 0;
  }

  if (strcmp("$pc", s) == 0) {
    *success = true;
    return (word_t)cpu.pc;
  }

  for (int i = 0; i < 32; ++i) {
    assert(*(s + 1) != '\0');
    if (strcmp(regs[i], s + 1) == 0) {
      *success = true;
      return gpr(i);
    }
  }

  panic("Failed to match reg: %s", s);
  *success = false;
  return 0;
}
