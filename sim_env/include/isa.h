#ifndef __ISA_H__
#define __ISA_H__

#include <isa-def.h>

// The macro `__GUEST_ISA__` is defined in $(CFLAGS).
// It will be expanded as "x86" or "mips32" ...
typedef concat(__GUEST_ISA__, _CPU_state) CPU_state;
typedef concat(__GUEST_ISA__, _ISADecodeInfo) ISADecodeInfo;

//monitor
void init_isa();

// regfile
extern CPU_state cpu;
void isa_reg_display();
word_t isa_reg_str2val(const char *name, bool *success);

// exec
struct Decode;
void isa_exec_once(struct Decode *s);

// memory
// MMU-related

// interruption/exception

// difftest
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc);
void isa_difftest_attach();
void isa_difftest_sync_gprs(CPU_state *dut); /* patch for difftest */
#endif
