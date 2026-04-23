#include <cpu/cpu.h>
#include <cpu/decode.h>
#include <cpu/difftest.h>
#include <locale.h>
#include <isa.h>

#define MAX_INST_TO_PRINT 10

CPU_state cpu = {};
uint64_t g_nr_guest_inst = 0;
uint64_t g_nr_clocks = 0;
static uint64_t g_timer = 0;
static bool g_print_step = false;

// void device_update();

static void exec_once(Decode *s) {
  isa_exec_once(s); // npc_exec_once inside
  cpu.pc = s->dnpc; /* update */
}

static void trace_and_difftest(Decode *_this, vaddr_t dnpc) {

#ifdef CONFIG_ITRACE
  void itrace_record(Decode *s);
  itrace_record(_this);
#endif
#ifdef CONFIG_FTRACE
  void ftrace_record(Decode *s);
  ftrace_record(_this);
#endif

#ifdef CONFIG_ITRACE
  log_write("%s\n", _this->logbuf);
#endif
  if (g_print_step) { IFDEF(CONFIG_ITRACE, puts(_this->logbuf)); }
  IFDEF(CONFIG_DIFFTEST, difftest_step(_this->pc, dnpc));
#ifdef CONFIG_WATCHPOINT
  extern bool wp_test_update();
  if (wp_test_update()) {
    npc_state.state = NPC_STOP;
  }

#endif
}

static void execute(uint64_t n) {
  Decode s = {0};
  for(; n > 0; n --) {
    exec_once(&s);
    g_nr_guest_inst ++;
    trace_and_difftest(&s, cpu.pc);
    if (npc_state.state != NPC_RUNNING) break;
  }
}

static void statistic() {
  IFNDEF(CONFIG_TARGET_AM, setlocale(LC_NUMERIC, ""));
#define NUMBERIC_FMT "%'" PRIu64
  Log("host time spent = " NUMBERIC_FMT " us", g_timer);
  Log("total guest instructions = " NUMBERIC_FMT, g_nr_guest_inst);
  if (g_timer > 0) Log("simulation frequency = " NUMBERIC_FMT " inst/s", g_nr_guest_inst * 1000000 / g_timer);
  else Log("Finish running in less than 1 us and can not calculate the simulation frequency");
  if (g_nr_clocks > 0) Log("IPC: %f", (double)g_nr_guest_inst / (double)g_nr_clocks);
#ifdef CONFIG_USE_PERF
  void perfcount_display(void);
  perfcount_display();
#endif // CONFIG_USE_PERF
}

void cpu_exec(uint64_t n) {
  g_print_step = (n < MAX_INST_TO_PRINT);
  switch (npc_state.state) {
  case NPC_END: case NPC_ABORT: case NPC_QUIT: 
    printf("Program execution has ended. To restart the program, exit NPC and run again.\n");
    return;
  default:
    npc_state.state = NPC_RUNNING;
    break;
  }

  uint64_t time_start = get_time();
  execute(n);
  uint64_t time_end = get_time();
  g_timer += time_end - time_start;

  switch (npc_state.state) {
    case NPC_RUNNING: npc_state.state = NPC_STOP; break;
    case NPC_END: case NPC_ABORT:
    Log("npc: %s at pc = " FMT_WORD,
      (npc_state.state == NPC_ABORT ? ANSI_FMT("ABORT", ANSI_FG_RED) :
        (npc_state.halt_ret == 0 ? 
          ANSI_FMT("HIT GOOD TRAP", ANSI_FG_GREEN) :
          ANSI_FMT("HIT BAD TRAP", ANSI_FG_RED))),
      npc_state.halt_pc); 
#ifdef CONFIG_ITRACE  
      void iringbuf_dump();
      if (npc_state.halt_ret != 0) iringbuf_dump();
#endif
    // fall through
    case NPC_QUIT: statistic();
  }
}