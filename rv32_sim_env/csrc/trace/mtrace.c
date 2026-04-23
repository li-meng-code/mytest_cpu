#include <common.h>
#include <isa.h>

#ifdef CONFIG_MTRACE

static FILE *mtrace_fp;

void init_mtrace(const char *mtrace_file) {
    mtrace_fp = stdout; /* default when user doesn't set a output file */
    if (mtrace_file != NULL) {
        FILE *fp = fopen(mtrace_file, "w");
        Assert(fp, "Cannot open '%s'", mtrace_file);
        mtrace_fp = fp;
    }
    Log("mtrace is written to %s", mtrace_file ? mtrace_file : "stdout");
    Log("Memory access trace: " ANSI_FMT("ON", ANSI_FG_GREEN));
}

inline static char len2char(int len) {
  switch(len) {
    case 1: return 'b';
    case 2: return 'h';
    case 4: return 'w';
    default: assert(0);
  }
}

inline static void mtrace_record(vaddr_t addr, int len, char type, word_t data) {
    Assert(mtrace_fp, "mtrace write failed");
    assert(type == 'w' || type == 'r');
    char buf[128];
    char *p = buf;
    p += snprintf(p, sizeof(buf), "[mtrace] %c%c@" FMT_PADDR ": addr=" FMT_PADDR " ", type, len2char(len), cpu.pc, addr);
    p += snprintf(p, sizeof(buf) - (p - buf), "data=" FMT_WORD, data);
    snprintf(p, sizeof(buf) - (p - buf), "\n");
    fprintf(mtrace_fp, "%s", buf);
    fflush(mtrace_fp);
}

void mtrace_w_record(vaddr_t addr, int len, word_t data) {
    mtrace_record(addr, len, 'w', data);
}

void mtrace_r_record(vaddr_t addr, int len, word_t data) {
    mtrace_record(addr, len, 'r', data);
}

#endif /* CONFIG MTRACE */
