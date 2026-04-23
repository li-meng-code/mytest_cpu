#include <common.h> /* <autoconf.h> is in there */
#include <cpu/cpu.h>
#include <cpu/decode.h>

#ifdef CONFIG_ITRACE

#define IRINGBUF_DEPTH 16
static struct InstructionRingbuffer {
  char buf[IRINGBUF_DEPTH][LOGBUF_LEN];
  unsigned int head;
  unsigned int tail;
} iringbuf = {0};

static void iringbuf_write(char */*restrict*/ ibuf) {
  strncpy(iringbuf.buf[iringbuf.tail], ibuf, LOGBUF_LEN - 1);
  iringbuf.buf[iringbuf.tail][LOGBUF_LEN - 1] = '\0';
  iringbuf.tail = (iringbuf.tail + 1) % IRINGBUF_DEPTH;
  if (iringbuf.head == iringbuf.tail) { /* full */
    iringbuf.head = (iringbuf.head + 1) % IRINGBUF_DEPTH;
  }
}

void iringbuf_dump() {
  unsigned int p;
  printf("\033[31mINSTRUCTIO RINGBUFFER DUMP:\033[0m\n");
  for (p = iringbuf.head; p != iringbuf.tail; p = (p + 1) % IRINGBUF_DEPTH) {
    if ((p + 1) % IRINGBUF_DEPTH == iringbuf.tail) break;
    puts(iringbuf.buf[p]);
  }
  printf("[\033[31m%s\033[0m] --> There's something wrong.\n", iringbuf.buf[p]);
}

void itrace_record(Decode *s) {
  char *p = s->logbuf;
  p += snprintf(p, sizeof(s->logbuf), FMT_WORD ":", s->pc);
  int ilen = 4; // riscv32
  int i;
  uint8_t *inst = (uint8_t *)&s->isa.inst;
  for (i = ilen - 1; i >= 0; i --) {
    p += snprintf(p, 4, " %02x", inst[i]);
  }
  int ilen_max = 4; // riscv32
  int space_len = ilen_max - ilen;
  if (space_len < 0) space_len = 0;
  space_len = space_len * 3 + 1;
  memset(p, ' ', space_len);
  p += space_len;

  void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);
  disassemble(p, s->logbuf + sizeof(s->logbuf) - p,
      MUXDEF(CONFIG_ISA_x86, s->snpc, s->pc), (uint8_t *)&s->isa.inst, ilen);

  /*write one log to the ringbuf*/
  iringbuf_write(s->logbuf);
}
#endif /* CONFIG_ITRACE */
