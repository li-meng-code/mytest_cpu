#include <common.h>
#include <getopt.h>
#include <memory/paddr.h>
#include <cpu/cpu.h>
#include <sim/npc-ctrl.h>

/****** IMPROTANT *******
 * npc boots @ 0x80000000
 * soc boots @ 0x20000000 
 ************************/

static char *log_file = NULL;
static char *img_file = NULL;
static char *boot_file = NULL;
static char *wave_file = NULL;
static char *mtrace_file = NULL;
static char *elf_file = NULL;
static char *ftrace_file = NULL;
static char *diff_so_file = NULL;
static int difftest_port = 1234;

void init_isa(); 
void init_srand();
void init_log(const char *log_file);
void init_mem();
void init_sdb();
void init_disasm();
void init_mtrace(const char *mtrace_file);
void init_ftrace(const char *ftrace_file, const char *elf_file);
void init_difftest(char *ref_so_file, long img_size, int port);
void init_npc(int argc, char *argv[], const char *wave_file);

void sdb_set_batch_mode();

static long load_img(const char *bin_file) {
  if (img_file == NULL) {
    Log("No image file is specified, use the built-in image.");
    return 4096;
  }
  FILE *fp = fopen(bin_file, "r");
  Assert(fp, "Cannot open file '%s'", bin_file);
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  Log("Reading binary file '%s' (%lu bytes).", bin_file,  size);
  size_t ret = fread(guest_to_host(RESET_VECTOR), 1, size, fp);
  Assert(ret == size, "Cannot read file '%s'", bin_file);
  return size;
}

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"wave"     , required_argument, NULL, 'w'},
    {"mtrace"   , required_argument, NULL, 'm'},
    {"ftrace"   , required_argument, NULL, 'f'},
    {"elf"      , required_argument, NULL, 'e'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:m:f:e:w:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'w': wave_file = optarg; break;
      case 'm': mtrace_file = optarg; break;
      case 'f': ftrace_file = optarg; break;
      case 'e': elf_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 1: img_file = optarg; return 0;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-m,--mtrace=FILE        output memory access trace to FILE\n");
        printf("\t-f,--ftrace=FILE        output function call trace to FILE\n");
        printf("\t-w,--wave=FILE          output waveform trace to FILE\n");
        printf("\t-e,--elf=FILE           elf file for function call tracing\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

static void welcome() {
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s-NPC!\n", ANSI_FMT("riscv32", ANSI_FG_RED ANSI_BG_YELLOW));
  printf("For help, type \"help\"\n");
}
void init_monitor(int argc, char *argv[]) {

  parse_args(argc, argv);

  init_srand();

  init_log(log_file);

  init_mem();

  init_isa();

  long img_size = load_img(img_file);

  init_npc(argc, argv, wave_file);

  init_sdb();

  IFDEF(CONFIG_ITRACE, init_disasm());
  IFDEF(CONFIG_ITRACE, Log("Instruction trace: " ANSI_FMT("ON", ANSI_FG_GREEN)));
  IFDEF(CONFIG_MTRACE, init_mtrace(mtrace_file));
  IFDEF(CONFIG_FTRACE, init_ftrace(ftrace_file, elf_file));
  IFDEF(CONFIG_DIFFTEST, init_difftest(diff_so_file, img_size, difftest_port));

  welcome();
}
