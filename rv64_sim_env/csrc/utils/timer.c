#include <common.h>
#include <sys/time.h>

static uint64_t boot_time; 

static uint64_t get_host_time() {
  struct timeval now;
  gettimeofday(&now, NULL);
  uint64_t us = (uint64_t)now.tv_sec * 1000000 + now.tv_usec;
  return us; 
}

uint64_t get_time() {
  if (boot_time == 0) boot_time = get_host_time();
  uint64_t now = get_host_time();
  return now - boot_time;
}

void init_srand() {
  srand(get_host_time());
}


