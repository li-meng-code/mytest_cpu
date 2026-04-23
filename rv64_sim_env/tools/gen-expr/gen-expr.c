/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static int buf_valid;
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

inline static uint32_t choose(uint32_t n) {
  return rand() % n;
}

static void gen(const char* str) {
  if(!buf_valid) {
    return;
  }
  size_t str_end = strlen(buf);
  if (strlen(buf) + strlen(str) >= sizeof(buf)) {
    buf_valid = 0;
    return;
  }

  sprintf(buf + str_end, "%s", str);

}

#define MAX_SPACE_NUM 4
inline static void gen_space() {
  char spaces[MAX_SPACE_NUM + 1] = {' '};
  int space_num = choose(MAX_SPACE_NUM + 1);
  spaces[space_num] = '\0';
  gen(spaces);
}


static void gen_num() {
  char num_str[16];
  num_str[0] = '\0';
  uint32_t num = choose((uint32_t) -1);
  sprintf(num_str, "%uu", num); 
  assert(strlen(num_str) < sizeof(num_str));
  gen(num_str);
}

static void gen_op() {
  switch(choose(4)) {
    case 0: gen("+"); break;
    case 1: gen("-"); break;
    case 2: gen("*"); break;
    case 3: gen("/"); break;
    default: assert(0);
  }
}

static void __gen_rand_expr(int reset) {
  if (reset) buf[0] = '\0';
  switch(choose(3)) {
    case 0: gen_space(); gen_num(); gen_space(); break;
    case 1: gen_space(); gen("("); gen_space(); __gen_rand_expr(0); gen_space(); gen(")"); gen_space(); break;
    default:gen_space(); __gen_rand_expr(0); gen_space(); gen_op(); __gen_rand_expr(0); gen_space(); break;
  }
}

static void gen_rand_expr() {
  int i;
  for (i = 0; i < 100; i++) {
  buf_valid = 1;
  __gen_rand_expr(1);
  if (buf_valid) return;
  }
  puts("gen_rand_expr failed after 100 tries\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    gen_rand_expr(); // reset

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -Wall -Werror -o /tmp/.expr");
    if (WIFEXITED(ret)) { // normally exited, not signaled
      if (WEXITSTATUS(ret) == 1) { // failed to compile
        i--;
        continue;
      } else {
        assert (WEXITSTATUS(ret) == 0);
      }
    } 
    else {
      assert(0);
    }

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u=%s\n", result, buf);
  }
  return 0;
}
