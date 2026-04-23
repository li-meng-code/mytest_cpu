#include <common.h>

/* TODO: compatibility for rv64 */

#ifdef CONFIG_FTRACE

#include <cpu/decode.h>

typedef enum {CALL_NONE = -1, CALL_START, CALL_DIRECT, CALL_INDIRECT, CALL_TAIL} call_type;
typedef struct {
    const char *caller; /* for the first entry, initialize to "machine" */
    const char *callee; /* for the first entry, initialize to "start" */
    call_type type; /* for the first entry, initialize to  START */
    
} CallStackEntry;
#define MAX_CALL_DEPTH 1024
static CallStackEntry call_stack[MAX_CALL_DEPTH];
static int call_tos = 0;
#define CALLSTACK_TOP() (call_stack[call_tos - 1])

static FILE *ftrace_fp;

int ftrace_elf_analyze(const char *elf_file); /* ftrace-elf.c */
const char *func_hash_lookup(paddr_t addr); 

static void call_stack_push(const char *caller, const char *callee, call_type type) {
    Assert(call_tos < MAX_CALL_DEPTH, "ftrace call stack out of max depth");
    call_stack[call_tos++] = (CallStackEntry){.caller = caller, .callee = callee, .type = type};
}

inline static void call_stack_pop() {
    Assert(call_tos > 0, "ftrace call is already empty, but pop was called");
    --call_tos;
}

inline static void call_stack_replace_callee(const char *callee) {
    /* if a tail call happened */
    CALLSTACK_TOP().callee = callee;
}

#ifdef CONFIG_FTRACE_INDENT
static unsigned int indent_level = -1;

static inline void ftrace_indent(void) {
    for (unsigned int i = 0; i < indent_level; i++) {
        fprintf(ftrace_fp, "  ");
    }
}

static inline void ftrace_add_indent(void) {
    indent_level++;
    ftrace_indent();
}

static inline void ftrace_reduce_indent(void) {
    if (indent_level > 0) {
        indent_level--;
    }
    ftrace_indent();
}


#else
#define ftrace_indent()        ((void)0)
#define ftrace_add_indent()    ((void)0)
#define ftrace_reduce_indent() ((void)0)
#endif

#define __ftrace_get_bits(num, h, l) ((num >> l) & ((1 << (h - l + 1)) - 1)) 
void ftrace_record(Decode *s) {
    /* zero-x0-hard wired zero  ra-x1-return address */
    const word_t mask_jalr = 0b00000000000000000111000001111111;
    const word_t key_jalr  = 0b00000000000000000000000001100111;
    const word_t mask_jal  = 0b00000000000000000000000001111111;
    const word_t key_jal   = 0b00000000000000000000000001101111;
    enum {NONE, JAL, JALR} jump_type = NONE;
    int immI, rs1, rd;
    rs1  = __ftrace_get_bits(s->isa.inst, 19, 15);
    rd   = __ftrace_get_bits(s->isa.inst, 11, 7);
   /* find out the jump type */
    if ((s->isa.inst & mask_jal) == key_jal) jump_type = JAL;
    else if ((s->isa.inst & mask_jalr) == key_jalr) {
        jump_type = JALR;
        immI = __ftrace_get_bits(s->isa.inst, 31, 20);
    }
    else return;
    /* consider if it's a function return */
    if (jump_type == JALR && (rd ==0 && rs1 == 1 && immI == 0)) {
        /* it's a return */
        fprintf(ftrace_fp, "[ftrace] ");
        ftrace_reduce_indent();
        fprintf(ftrace_fp, "RETURN %s\n", CALLSTACK_TOP().callee);
        call_stack_pop();
        return;
    }

    /* consider if it's a function call */
    paddr_t target = s->dnpc;
    call_type ct = CALL_NONE;
    const char* callee = func_hash_lookup(target);
    if (!callee) return; /* it's not a function call */
    if (rd == 1) { /* normal call */
        if (jump_type == JAL) ct = CALL_DIRECT;
        else if (jump_type == JALR) ct = CALL_INDIRECT;
        call_stack_push(CALLSTACK_TOP().callee, callee, ct);
        fprintf(ftrace_fp, "[ftrace] ");
        ftrace_add_indent();
        fprintf(ftrace_fp, "CALL %s --> %s@0x%08x (%s)\n", 
            CALLSTACK_TOP().caller, CALLSTACK_TOP().callee,
            target,
            ct == CALL_DIRECT ? "direct" : "indirect"
        );
    }
    else if (rd == 0) {
        ct = CALL_TAIL;
        fprintf(ftrace_fp, "[ftrace] ");
        ftrace_indent();
        fprintf(ftrace_fp, "TAIL %s --> %s@0x%08x\n", CALLSTACK_TOP().callee, callee, target);
        call_stack_replace_callee(callee);
    }
    /* ct should not be a CALL_START type */
    Assert(ct > 0, "A non-call instruction reached to a funciton");
}

void init_ftrace(const char *ftrace_file, const char *elf_file) {
    /* init ftrace file */
    ftrace_fp = stdout;
    if (ftrace_file != NULL) {
        FILE *fp = fopen(ftrace_file, "w");
        Assert(fp, "Cannot open '%s'", ftrace_file);
        ftrace_fp = fp;
    }
    Log("ftrace is written to %s", ftrace_file ? ftrace_file : "stdout");
    /* analyzing function */
    if (!elf_file) {
        Log("no elf file is loaded, ftrace will not be initialized");
        if (ftrace_fp != NULL && ftrace_fp != stdout) {
            fclose(ftrace_fp);
        }
        return;
    }
    Log("ftrace is analyzing elf '%s'", elf_file);
    Assert(ftrace_elf_analyze(elf_file) == 0, "failed to anaylyze '%s'", elf_file);
    /* initialize the stack */
    call_stack_push("machine", "_start", CALL_START);
    Log("Function call trace: " ANSI_FMT("ON", ANSI_FG_GREEN));
}

#endif
