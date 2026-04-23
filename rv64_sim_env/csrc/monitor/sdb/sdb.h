#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);


#define NR_WP 32
#define MAX_LEN_EXPR 128

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;
  struct watchpoint *prev;
  char e[MAX_LEN_EXPR + 1];
  bool used;
  word_t val;
  /* TODO: Add more members if necessary */
} WP;

void new_wp(char* e);
void free_wp(int N);
void wp_display();
bool wp_test();

#endif
