#include <common.h>
#include <memory/paddr.h>
#include <utils.h>
#include <memory>
#include <assert.h>
#include <cstdio>

// #include <VTop.h>
#include <verilated_vcd_c.h>
#include <verilated.h>
#include <sim/npc-ctrl.h>

#ifndef TOP_MODULE
#error "TOP_MODULE not defined!"
#endif

#include str(concat(V,TOP_MODULE.h))
#include str(concat(V,concat(TOP_MODULE,__Dpi.h)))

typedef concat(V,TOP_MODULE) npc_t;

#ifdef __NVBOARD__
#include <nvboard.h>
void nvboard_bind_all_pins(npc_t* npc);
#endif

std::unique_ptr<npc_t> npc;

#ifdef CONFIG_WAVE_TRACE
static std::unique_ptr<VerilatedVcdC> tfp;
#endif

extern uint64_t g_nr_clocks;

#ifdef CONFIG_WAVE_TRACE
static void wave_dump(uint64_t step) {
  if (tfp) tfp->dump(step);
}
#else
static void wave_dump(uint64_t step) {
  return;
}
#endif

static void single_cycle() {
  npc->clock = 0;
  npc->eval();
  wave_dump(g_nr_clocks * 2);
#ifdef __NVBOARD__
  nvboard_update(); /* uart should sample every single cycle */
#endif
  npc->clock = 1;
  npc->eval();
  wave_dump(g_nr_clocks * 2 + 1);
  g_nr_clocks++;
}

void set_dpi_scope(const char *name) {
  const svScope scope = svGetScopeFromName(name);
  assert(scope);
  svSetScope(scope);
}

void npc_exec_once(Decode *s) {
#ifdef IS_YSYX_SOC
  set_dpi_scope("TOP.ysyxSoCFull.asic.cpu.cpu");
#else
  set_dpi_scope("TOP.Top");
#endif

  while (!sim_commit_valid()) {
    single_cycle();
  }

  unsigned long long pc64   = 0;
  unsigned long long dnpc64 = 0;
  unsigned int       inst32 = 0;

  sim_commit_context(&pc64, &dnpc64, &inst32);

  s->pc       = (vaddr_t)pc64;
  s->dnpc     = (vaddr_t)dnpc64;
  s->isa.inst = (uint32_t)inst32;

  single_cycle();
}

static void npc_reset(int n) {
  npc->reset = 1;
  while (n-- > 0) {
    single_cycle();
  }
  npc->reset = 0;
}

void npc_cleanup() {
#ifdef CONFIG_WAVE_TRACE
  if (tfp) tfp->close();
  tfp.reset();
#endif
  npc.reset();
#ifdef __NVBOARD__
  nvboard_quit();
#endif
}

void init_npc(int argc, char *argv[], const char *wavefile) {
  Verilated::commandArgs(argc, argv);
  npc = std::make_unique<npc_t>();
#ifdef CONFIG_WAVE_TRACE
  tfp = std::make_unique<VerilatedVcdC>();
  Verilated::traceEverOn(true);
  npc->trace(tfp.get(), 99);
  Assert(wavefile, "wavefile is NULL.");
  tfp->open(wavefile);
#endif
#ifdef __NVBOARD__
  nvboard_bind_all_pins(npc.get());
  nvboard_init();
#endif
  npc_reset(20);
}
