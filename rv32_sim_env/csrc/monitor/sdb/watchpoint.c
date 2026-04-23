#include "sdb.h"

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    wp_pool[i].prev = (i == 0 ? NULL : &wp_pool[i - 1]);
    wp_pool[i].e[0] = '\0';
    wp_pool[i].used = false;
    wp_pool[i].val = 0;
  }
  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

static int unused = NR_WP;
void new_wp(char *e) {
  if (free_ == NULL) {
    printf("Cannot allocate more watchpoints: limit of %d reached.\n", NR_WP);
    return;
  } 
  if (e == NULL) {
    printf("The eession received by new_wp is null.\n");
    return;
  }
  if (strlen(e) >= MAX_LEN_EXPR) {
    printf("The string length of expression (%lu) received by new_wp is too long: limit %lu reached.\n", strlen(e), (size_t) MAX_LEN_EXPR);
    return;
  }

  bool success = false;
  word_t val = expr(e, &success);
  if (!success) {
    printf("Invalid expression received by new_wp!\n");
    return;
  }

  WP *new_wp = free_;
  free_ = free_->next;

  new_wp->next = head;
  new_wp->prev = NULL;
  if (head) { head->prev = new_wp; }
  head = new_wp;

  assert(strcpy(new_wp->e, e));
  new_wp->used = true;
  new_wp->val = val;

  unused--;
  assert(unused >= 0);

}

bool wp_test_update() {
  bool test = false;
  for (WP *ptr = head; ptr != NULL; ptr = ptr->next) {
    bool success = false;
    word_t new_val = expr(ptr->e, &success);
    assert(success);
    if (new_val != ptr->val) {
      printf("\033[35mHit watchpoint\033[0m %d:\told value: %u\tnew value: %u\texpr: {%s}\n", ptr->NO, ptr->val, new_val, ptr->e);
      test |= true;
    }
    ptr->val = new_val;
  }
  return test;
}

void wp_display() {
  for (WP *ptr = head; ptr != NULL; ptr = ptr->next) {
    printf("%d\t%s\n", ptr->NO, ptr->e);
  }
  printf("Free watchpoints: %d/%d\n", unused, NR_WP);
}

inline static bool check_wp_idx(int N) {
  if (N > NR_WP || N < 0) return false;
  return true;
}

void free_wp(int N) {
  if (!check_wp_idx(N)) {
    printf("The watchpoint number out of range [0-%d]\n", NR_WP);
    return;
  }
  if (!wp_pool[N].used) {
    printf("The watchpoint is already freed.\n");
    return;
  }
  WP *old_wp = &wp_pool[N];

  if (old_wp->prev) { old_wp->prev->next = old_wp->next; } // if not head
  else { head = head->next; }

  if (old_wp->next) { old_wp->next->prev = old_wp->prev; } // if not tail

  old_wp->next = free_;
  old_wp->prev = NULL;
  if (free_) { free_->prev = old_wp; }
  free_ = old_wp;

  old_wp->e[0] = '\0';
  old_wp->used = false;

  unused ++;

  printf("Watchpoint %d freed.\n", N);

  return;
}

