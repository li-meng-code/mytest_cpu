#include <common.h>
#include "local-include/reg.h"

#ifndef TOP_MODULE
#error "TOP_MODULE not defined!"
#endif

#include str(concat(V, concat(TOP_MODULE, __Dpi.h)))

void set_dpi_scope(const char *name);

word_t gpr(unsigned int idx) {
#ifdef IS_YSYX_SOC
  set_dpi_scope("TOP.ysyxSoCFull.asic.cpu.cpu");
#else
  set_dpi_scope("TOP.Top");
#endif
  return (word_t)sim_regfile_read(check_reg_idx(idx));
}
