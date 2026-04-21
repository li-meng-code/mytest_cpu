#ifndef __SIM_NPC_CTRL_H__
#define __SIM_NPC_CTRL_H__

#include <common.h>
#include <memory/vaddr.h>
#include <cpu/decode.h>

#ifdef __cplusplus
extern "C" {
#endif

/* unlike nemu, npc's exec_once fuctionality is not isa-specified */
void npc_exec_once(Decode *s);
void npc_cleanup();
#ifdef __cplusplus
}
#endif
#endif
