#include <memory/paddr.h>
#include <memory/vaddr.h>

#ifdef CONFIG_MTRACE
void mtrace_w_record(vaddr_t addr, int len, word_t data);
void mtrace_r_record(vaddr_t addr, int len, word_t data);
#endif

word_t vaddr_ifetch(vaddr_t addr, int len) {
  return paddr_read(addr, len);
}

word_t vaddr_read(vaddr_t addr, int len) {
  static word_t data;
  data = paddr_read(addr, len);
#ifdef CONFIG_MTRACE
  mtrace_r_record(addr, len, data); 
#endif
  return data;
}

void vaddr_write(vaddr_t addr, int len, word_t data) {
#ifdef CONFIG_MTRACE
  mtrace_w_record(addr, len, data);
#endif
  paddr_write(addr, len, data);
}
