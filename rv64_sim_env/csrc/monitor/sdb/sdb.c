#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <memory/vaddr.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <common.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }
  char prompt[64];
  line_read = readline("(npc) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  set_npc_state(NPC_QUIT, cpu.pc, 0);
  return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_p(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */

  { "si", "si [N]: Step through N instructions and pause; defaults to 1 if N is not specified.", cmd_si },
  { "info", "info <SUBCMD>:\n\tinfo r: print register status.\n\tinfo w: print information of watchpoints.", cmd_info },
  { "x", "x <N> <EXPR>: Dump N consecutive 32-bit words in hex starting at address EXPR.", cmd_x},
  { "p", "p <EXPR>: Evaluate EXPR in the current context and output its value.", cmd_p},
  { "w", "w <EXPR>: Pause program execution when the value of the expression EXPR changes.", cmd_w},
  { "d", "d <N>: Remove watchpoint N.", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {

  long steps = 0;
  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    // go 1 step forward.
    steps = 1;
  } 
  else { // the argument is supposed to be the number of steps forward.
    steps = strtol(arg, NULL, 0);
    if (steps <= 0) {
      printf("The argument of si should be a positive number.\n");
      return 0; // reuturn and do nothing.
    }
  }

  cpu_exec(steps);

  return 0;
} 

static int cmd_info(char *args) {

  char *arg = strtok(NULL, " ");

  if (arg == NULL) {
    printf("\'info\' needs an argument.\n");
  } 
  else {
    switch(*arg) {

      case 'r': 
        isa_reg_display();
        break;

      case 'w':
        wp_display();
        break;

      default: 
        printf("Invalid argument (%s) of \'info\'.\n", arg);
        break;
    }
  }
  return 0;
}

static int cmd_x(char *args) {
  long vaddr;
  long N;
  // extract the first argument
  char *arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("\'x\' needs more arguments.\n");
    return 0;
  } 
  // check the first argument
  if ((N = strtol(arg, NULL, 0)) <= 0) {
    printf("The number of bytes to read (%s) is not valid\n", arg);
    return 0;
  }

  // extract the second argument
  arg = strtok(NULL, " ");
  if (arg == NULL) {
    printf("\'x\' needs more arguments.\n");
    return 0;
  }
  // vaddr = strtol(arg, NULL, 0);
  bool success = false;
  vaddr = expr(arg, &success);

  word_t mem_read;
  vaddr_t vaddr_to_read;
  for(vaddr_t i = 0; i < N; ++i) {
    vaddr_to_read = vaddr + i*4; // increase by word
    // if (vaddr_to_read < PMEM_LEFT || vaddr_to_read > PMEM_RIGHT) {
      // printf("Address to read (" FMT_PADDR ") is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "]\n", vaddr_to_read, PMEM_LEFT, PMEM_RIGHT);
      // return 0;
    // }
    if (!in_storage(vaddr_to_read)) return 0;
    mem_read = vaddr_read(vaddr_to_read, 4); // len = 4, read 4 byte = 1 word
    printf(FMT_PADDR": " FMT_WORD"\n", vaddr_to_read, mem_read);
  }
  return 0;
}


static int cmd_p(char *args) {
  bool success = false;
  expr(args, &success);
  // assert(success);
  // TODO: format the values
  return 0;
}

static int cmd_w(char *args) {

  if (!args) {
    printf("Command w received a null pointer.\n");
    return 0;
  }
  printf("\033[32m Set watchpoint {%s}. \033[0m\n", args);
  new_wp(args);
  return 0;
}

static int cmd_d(char *args) {
  if (!args) {
    printf("Command d needs one argument!.\n");
  }
  printf("\033[31m Remove watchpoint {%s}. \033[0m\n", args);
  int N = strtod(args, NULL);
  free_wp(N);
  return 0;
}


void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }

}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
