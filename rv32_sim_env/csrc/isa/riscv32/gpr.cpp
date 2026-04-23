#include <common.h>
// #include <VTop.h>
// #include <VTop___024root.h> 
// #include <verilated_fst_c.h>
// #include <verilated.h>
// #include <sim/npc-ctrl.h>
#include <svdpi.h>
// #include <VTop__Dpi.h>
#include "local-include/reg.h"

#include str(concat(V,concat(TOP_MODULE,__Dpi.h)))

void set_dpi_scope(const char *name);

word_t gpr(unsigned int idx) {  
 /* (Remember that Verilator adds a “TOP” to the top of the module hierarchy.) */
  void set_dpi_scope(const char *name);
#ifdef IS_YSYX_SOC
  set_dpi_scope("TOP.ysyxSoCFull.asic.cpu.cpu");
#else
  set_dpi_scope("TOP.Top");
#endif
  return sim_regfile_read(check_reg_idx(idx));
}
