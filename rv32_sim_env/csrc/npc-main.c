#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <signal.h>

#include <sim/npc-ctrl.h>

void init_monitor(int argc, char *argv[]);
void sdb_mainloop();

void handle_exit(int signum) {
    npc_cleanup();
    exit(0);
}

int is_exit_status_bad();


int main(int argc, char *argv[]) {
  /* for safely creating wave.fst */
  signal(SIGINT, handle_exit);
  signal(SIGTERM, handle_exit);

  init_monitor(argc, argv); // init_npc

  sdb_mainloop();

  npc_cleanup();
  Log("Simulation finished.");

  return is_exit_status_bad();
}