#include <common.h>
#include <svdpi.h>


// TODO: add this to kconfig

#ifdef CONFIG_USE_PERF
#include str(concat(V,concat(TOP_MODULE,__Dpi.h)))

void set_dpi_scope(const char *name);

extern uint64_t g_nr_clocks;
extern uint64_t g_nr_guest_inst;

static double hit_p = 0;
static double miss_p = 0;
static long long unsigned hit_access = 0;
static long long unsigned miss_access = 0;

static const char *perfcount_names[] = {
    "fetched", "executed", "if2id", "loaded", "stored",
    "dec_load", "dec_store", "dec_alu", "dec_jump", "dec_csr",
    "dec_I", "dec_R", "dec_U", "dec_S", "dec_J", "dec_B", "dec_SYS",
    "commit",
    "if_ar2r", "if_wb2ar",
    "if_r", "if_ar",
    "iHit", "iMiss"
};

#define NR_PERFCNT (sizeof(perfcount_names) / sizeof(perfcount_names[0]))

static void perfcount_print(unsigned int idx) {
  /* (Remember that Verilator adds a “TOP” to the top of the module hierarchy.) */
  void set_dpi_scope(const char *name);
#ifdef IS_YSYX_SOC
  set_dpi_scope("TOP.ysyxSoCFull.asic.cpu.cpu");
#else
  set_dpi_scope("TOP.Top");
#endif

  unsigned long long cnt, lacc;
  sim_perfcount_read(NR_PERFCNT - 1 - idx, &cnt, &lacc); /* It's not easy to do this reversion in system verilog */
  double epc = (double)cnt / (double)g_nr_clocks;
  double portion = (double)cnt / (double)g_nr_guest_inst * 100;
  double avg_ltc = cnt == 0 ? -1 : (double)lacc / (double)cnt;
  char avg_str[10];
  sprintf(avg_str, "%-4.2f", avg_ltc);
  printf(ANSI_FMT("[ %-9s ] %-6llu  %-6.2f%%   %-5.3f   %-8llu  %-5s\n", ANSI_FG_GREEN), perfcount_names[idx], cnt, portion, epc, lacc, avg_ltc > 0 ? avg_str : "NaN");

  if (strcmp("iHit", perfcount_names[idx])) {
    hit_p = portion;
    hit_access = avg_ltc;
  }

  if (strcmp("iMiss", perfcount_names[idx])) {
    miss_p = portion;
    miss_access = avg_ltc;
  }
}

void perfcount_display(void) {  
  printf(ANSI_FMT("[ %-9s ] %-6s  %-4s   %-5s   %-8s  %-4s\n", ANSI_FG_GREEN), "evt", "cnt", "portion", "epc", "ltc", "avg.ltc");
  for (int i = 0; i < NR_PERFCNT; ++i) {
    perfcount_print(i);
  }
  printf(ANSI_FMT("AMAT: iHit_T*iHit%% + iMiss_T*iMiss%% = %f\n", ANSI_FG_GREEN), (hit_access*hit_p + miss_access*miss_p)/100);
}

#endif