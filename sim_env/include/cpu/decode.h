#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>

#ifdef CONFIG_ITRACE
#define LOGBUF_LEN 128
#endif

typedef struct Decode {
  vaddr_t pc;
  vaddr_t dnpc;
  ISADecodeInfo isa;
  IFDEF(CONFIG_ITRACE, char logbuf[LOGBUF_LEN]);
} Decode;


#endif
