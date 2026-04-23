#include <common.h>
#include <generated/autoconf.h>
#include <capstone/capstone.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static csh handle;

void init_disasm(const char *triple) {
  cs_err err;

#if defined(CONFIG_XLEN) && (CONFIG_XLEN == 64)
  err = cs_open(CS_ARCH_RISCV, CS_MODE_RISCV64, &handle);
#else
  err = cs_open(CS_ARCH_RISCV, CS_MODE_RISCV32, &handle);
#endif

  assert(err == CS_ERR_OK);
  cs_option(handle, CS_OPT_DETAIL, CS_OPT_OFF);
}

void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
  cs_insn *insn = NULL;
  size_t count = cs_disasm(handle, code, nbyte, pc, 1, &insn);

  if (count == 0) {
    int off = snprintf(str, size, ".word 0x");
    for (int i = nbyte - 1; i >= 0 && off < size; --i) {
      off += snprintf(str + off, size - off, "%02x", code[i]);
    }
    return;
  }

  if (insn[0].op_str[0] != '\0') {
    snprintf(str, size, "%s %s", insn[0].mnemonic, insn[0].op_str);
  } else {
    snprintf(str, size, "%s", insn[0].mnemonic);
  }

  cs_free(insn, count);
}