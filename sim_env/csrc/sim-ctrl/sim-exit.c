#include <common.h>
#include <svdpi.h>
#include <stdio.h>

#include <cpu/cpu.h>

typedef enum {
  EXIT_GOOD     = 0,
  EXIT_BAD_CODE = 1,
  EXIT_BAD_INST = 2
} sim_event_t;

#ifdef __cplusplus
extern "C" {
#endif
// DPI-C
void sim_exit(sim_event_t sim_event, vaddr_t pc) {
  vaddr_t thispc = (vaddr_t)pc;
  switch(sim_event) {
    case EXIT_GOOD:
      set_npc_state(NPC_END, thispc, 0);
      break;
    case EXIT_BAD_CODE: case EXIT_BAD_INST:
      if (sim_event == EXIT_BAD_INST) { 
        printf(ANSI_FMT("BAD INSTRUCTION\n", ANSI_FG_RED));
      }
      set_npc_state(NPC_END, thispc, -1);
      break;
    default: assert(0);
  }
}
#ifdef __cplusplus

}
#endif
