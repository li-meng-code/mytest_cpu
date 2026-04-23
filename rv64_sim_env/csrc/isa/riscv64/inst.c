#include <cpu/decode.h>
#include <isa.h>
#include <sim/npc-ctrl.h>

void isa_exec_once(Decode *s) {
  npc_exec_once(s);
}
