#include <isa.h> /* extern CPU_state cpu for pc reading */
#include <cpu/difftest.h>
#include "../local-include/reg.h" /* gpr() function for general purpose register reading */

#define checkreg(name, ref, dut) do { \
  if (ref != dut) { \
    fprintf(stdout, "[difftest] Difftest failure @ pc = " FMT_PADDR "\n", pc); \
    fprintf(stdout, "[difftest] reg = %-2s | ref = " FMT_WORD " | dut = " FMT_WORD "\n", name, (word_t)ref, (word_t)dut); \
    goto failure; \
  } \
} while (0)

bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  int i;
  for (i = 0; i < 16; ++i) { /* RV32E */
    /* checkreg(reg_name(i), ref_r->gpr[i], cpu.gpr[i]); */
    checkreg(reg_name(i), ref_r->gpr[i], /*NPC*/gpr(i));
    /* unlike NEMU, NPC only supports gpr() function to read general purpose registers */
  }
  checkreg("pc", ref_r->pc, cpu.pc);
  return true;
failure:
  return false;
}

void isa_difftest_sync_gprs(CPU_state *dut) {
  for (int i = 0; i < RISCV_GPR_NUM; ++i) {
    dut->gpr[i] = gpr(i);
  }
}

void isa_difftest_attach() {
  return;
}