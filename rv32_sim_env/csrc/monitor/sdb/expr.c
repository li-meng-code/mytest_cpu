// #include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <isa.h>
#include <regex.h>
#include <memory/paddr.h>
#include <stdint.h>
#include <setjmp.h>

enum {
  TK_NOTYPE = 0, TK_LPAREN, TK_RPAREN,  
  TK_HEX_NUM, TK_DEC_NUM, TK_REG, // operand
  TK_EQ, TK_NEQ, TK_GE, TK_LE, TK_GT, TK_LT, // comparator
  TK_LSHIFT, TK_RSHIFT, // bit shift
  TK_AND, TK_OR, // logic (binary)
  TK_PLUS, TK_SUB, TK_MULT, TK_DIV, // binary
  TK_BIT_AND, TK_BIT_OR, TK_BIT_XOR, // bit-wise logic
  TK_BIT_REVERSE, // logic (unary)
  TK_POS, TK_NEG,  // unary
  TK_DEREF, // dereference
}; // THESE ARE also the INDICES of RULES;

enum { ASSOCI_VOID = 0, ASSOCI_LEFT, ASSOCI_RIGHT };
enum { CLS_VOID = 0, CLS_operand, CLS_BINARY, CLS_UNARY};

static struct rule {
  const char *regex;
  int token_type;
  int token_preced;
  int token_associ;
  int token_cls;
} rules[] = {
  {"[[:space:]]+", TK_NOTYPE, 0, ASSOCI_VOID, CLS_VOID},          // spaces
  {"\\(", TK_LPAREN, 0, ASSOCI_VOID, CLS_VOID},                   // left parenthesis
  {"\\)", TK_RPAREN, 0, ASSOCI_VOID, CLS_VOID},                   // right parenthesis
  {"0[xX][0-9a-fA-f]+", TK_HEX_NUM, 0, ASSOCI_VOID, CLS_operand},  // heximal number
  {"[0-9]+u*", TK_DEC_NUM, 0, ASSOCI_VOID, CLS_operand},           // numbers
  {"\\$(0|pc|ra|sp|gp|tp|t0|t1|t2|s0|s1|a0|a1|a2|a3|a4|a5|a6|a7|s2|s3|s4|s5|s6|s7|s8|s9|s10|s11|t3|t4|t5|t6)", 
    TK_REG, 0, ASSOCI_VOID, CLS_operand},                          // reg name
// --------------------------------------------------------------------------------
  {"==", TK_EQ, 7, ASSOCI_LEFT, CLS_BINARY},     // equal
  {"!=", TK_NEQ, 7, ASSOCI_LEFT, CLS_BINARY},    // not equal
  {">=", TK_GE, 6, ASSOCI_LEFT, CLS_BINARY},     // greater or equal
  {"<=", TK_LE, 6, ASSOCI_LEFT, CLS_BINARY},     // less or equal
  {">", TK_GT, 6, ASSOCI_LEFT, CLS_BINARY},      // greater
  {"<", TK_LT, 6, ASSOCI_LEFT, CLS_BINARY},      // less
  {"<<", TK_LSHIFT, 5, ASSOCI_LEFT, CLS_BINARY}, // left shift
  {">>", TK_RSHIFT, 5, ASSOCI_LEFT, CLS_BINARY}, // right shift
  {"&&", TK_AND, 11, ASSOCI_LEFT, CLS_BINARY},    // and
  {"\\|\\|", TK_OR, 12, ASSOCI_LEFT, CLS_BINARY}, // or
  {"\\+", TK_PLUS, 4, ASSOCI_LEFT, CLS_BINARY},  // plus
  {"-", TK_SUB, 4, ASSOCI_LEFT, CLS_BINARY},     // minus
  {"\\*", TK_MULT, 3, ASSOCI_LEFT, CLS_BINARY},  // multiply
  {"/", TK_DIV, 3, ASSOCI_LEFT, CLS_BINARY},     // division
  {"&", TK_BIT_AND, 8, ASSOCI_LEFT, CLS_BINARY},  // bit-wise and
  {"\\|", TK_BIT_OR, 10, ASSOCI_LEFT, CLS_BINARY},// bit-wise or
  {"\\^", TK_BIT_XOR, 9, ASSOCI_LEFT, CLS_BINARY},   // bit-wise xor
  {"~", TK_BIT_REVERSE, 2, ASSOCI_RIGHT, CLS_UNARY}, // bit reverse
  {"\\+", TK_POS, 2, ASSOCI_RIGHT, CLS_UNARY},   // positive
  {"-", TK_NEG, 2, ASSOCI_RIGHT, CLS_UNARY},     // negative
  {"\\*", TK_DEREF, 2, ASSOCI_RIGHT, CLS_UNARY},     // dereference
};

#define NR_REGEX ARRLEN(rules)
static void check_rule_idx(int idx) { assert(idx >= 0 && idx < NR_REGEX); };
static regex_t re[NR_REGEX] = {};
static jmp_buf jpb;
#define bad_expr_assert(cond) if(!(cond)) {/* assert(0); */ longjmp(jpb, 1); }

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

#define MAX_TOKEN_STR_LEN 31
typedef struct token {
  int type;
  char str[MAX_TOKEN_STR_LEN + 1];
  int preced;
  int associ;
  int cls;
} Token;

#define MAX_NR_TOKEN 65536 

static Token tokens[MAX_NR_TOKEN] __attribute__((used)) = {};
static int parentheses_match_table[MAX_NR_TOKEN] = {0};
static int nr_token __attribute__((used))  = 0;

static inline void check_token_idx(int idx) { assert(idx >= 0 && idx < MAX_NR_TOKEN); }
static inline bool is_unary_operator(int idx) { check_token_idx(idx); return (tokens[idx].cls == CLS_UNARY); }
static inline bool is_binary_operator(int idx) { check_token_idx(idx); return (tokens[idx].cls == CLS_BINARY); }
static inline bool is_operator(int idx) { check_token_idx(idx); return (tokens[idx].cls == CLS_UNARY || tokens[idx].cls == CLS_BINARY); }
static inline bool is_operand(int idx) { check_token_idx(idx); return tokens[idx].cls == CLS_operand; }
static inline int precedence(int idx) { check_token_idx(idx); return tokens[idx].preced; }
static inline int associactivity(int idx) { check_token_idx(idx); return tokens[idx].associ; }

static void record_token(int tk_idx, int rl_idx) {
  check_token_idx(tk_idx);
  check_rule_idx(rl_idx);
  tokens[tk_idx].type = rules[rl_idx].token_type;
  tokens[tk_idx].preced = rules[rl_idx].token_preced;
  tokens[tk_idx].associ = rules[rl_idx].token_associ;
  tokens[tk_idx].cls = rules[rl_idx].token_cls;
}

static bool make_token(char *e) {

  int position = 0;
  int i;
  int matched_idx = 0;
  regmatch_t pmatch;
  // int matched_idx = 0;
  int stack[MAX_NR_TOKEN] = {0};
  int top = 0;

  memset(parentheses_match_table, 0, sizeof(parentheses_match_table));
  memset(tokens, '\0', sizeof(tokens)); 

  // TODO();

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      assert(i == rules[i].token_type);
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {

        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        if (rules[i].token_type == TK_NOTYPE)
          break;

        if (nr_token > MAX_NR_TOKEN) {
          printf("The expression is too long (> %d non-space tokens).\n", MAX_NR_TOKEN);
          return false;
        }

        if (substr_len > MAX_TOKEN_STR_LEN) {
          printf("The token string is too long .\n");
          return false;
        }

        record_token(nr_token, i);
        assert(strncpy(tokens[nr_token].str, substr_start, substr_len));
        
        switch (tokens[nr_token].type) {
          case TK_LPAREN: stack[top++] = nr_token; break;

          case TK_RPAREN: 
            if (top <= 0) { printf("Parentheses check failed.\n"); return false; }
            matched_idx = stack[--top];
            if (tokens[matched_idx].type != TK_LPAREN) { printf("Parentheses check failed.\n"); return false; }
            parentheses_match_table[matched_idx] = nr_token;
            break;

          case TK_SUB: 
            if (nr_token == 0 || is_operator(nr_token - 1)) {
              record_token(nr_token, TK_NEG);
            } break;
          
          case TK_PLUS:
            if (nr_token == 0 || is_operator(nr_token - 1)) {
              record_token(nr_token, TK_POS);
            } break;
          case TK_MULT:
            if (nr_token == 0 || is_operator(nr_token - 1)) {
              record_token(nr_token, TK_DEREF);
            } break;
          default: break;
        }

        nr_token++;
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  // parentheses check result
  if (top != 0) {
    printf("Parentheses check failed. [stack top: %d]\n", top);
    return false;
  }

  return true;
}

static bool check_parentheses(unsigned int tk_start, unsigned int tk_end) {
  if (tokens[tk_start].type == TK_LPAREN && parentheses_match_table[tk_start] == tk_end) {
    return true;
  }
  else return false;
}

static int get_op_idx(unsigned int tk_start, unsigned int tk_end) {

  unsigned int pr = (unsigned int) 0;
  unsigned idx = -1;
  int type;
  unsigned i = tk_start;
  while (i < tk_end) {
    type = tokens[i].type;
    if (type == TK_LPAREN) { // skip
      i = parentheses_match_table[i] + 1;
      if (i > tk_end) {
        assert(i == tk_end + 1);
        break;
      }
      continue;
    }
    else if (is_operator(i) && (precedence(i) >= pr)) {
      if (precedence(i) == pr && associactivity(i) == ASSOCI_RIGHT) {
        i++;
        continue;
      } // else if ASSOCI_LEFT or <
      pr = precedence(i);
      idx = i;
    }
    i++;
  }
  bad_expr_assert(idx != (unsigned int) -1);
  bad_expr_assert(pr != 0);
  return idx;
}


static word_t eval(unsigned int tk_start, unsigned int tk_end) {

  bad_expr_assert(tk_end >= tk_start);

  // baseline
  if (tk_start == tk_end) {
    bad_expr_assert(is_operand(tk_start));
    switch (tokens[tk_start].type) {
      case TK_DEC_NUM: case TK_HEX_NUM: return strtoul(tokens[tk_start].str, NULL, 0); break;
      case TK_REG: {
        bool success = 0;
        word_t val = isa_reg_str2val(tokens[tk_start].str, &success);
        bad_expr_assert(success);
        return (word_t) val;
      }
      default: bad_expr_assert(0); break;
    }
  }

  // recursive
  if (check_parentheses(tk_start, tk_end)) {
    return eval(tk_start + 1, tk_end - 1);
  }

  int op_idx = get_op_idx(tk_start, tk_end);


  if (is_binary_operator(op_idx)) {

    bad_expr_assert(tk_start < op_idx && tk_end > op_idx);

    #define EXEC_BIOP(biop, val1, val2) ((word_t)(val1) biop (word_t)(val2))
    word_t val1 = 0;
    word_t val2 = 0;
    val1 = eval(tk_start, op_idx - 1);
    val2 = eval(op_idx + 1, tk_end);
    switch (tokens[op_idx].type) {
      case TK_PLUS:     return EXEC_BIOP(+,  val1, val2);
      case TK_SUB:      return EXEC_BIOP(-,  val1, val2);
      case TK_MULT:     return EXEC_BIOP(*,  val1, val2);
      case TK_DIV:      bad_expr_assert(val2 != 0); return EXEC_BIOP(/,  val1, val2);

      case TK_EQ:       return EXEC_BIOP(==, val1, val2);
      case TK_NEQ:      return EXEC_BIOP(!=, val1, val2);
      case TK_GT:       return EXEC_BIOP(>,  val1, val2);
      case TK_GE:       return EXEC_BIOP(>=, val1, val2);
      case TK_LT:       return EXEC_BIOP(<,  val1, val2);
      case TK_LE:       return EXEC_BIOP(<=, val1, val2);

      case TK_LSHIFT:   return EXEC_BIOP(<<, val1, val2);
      case TK_RSHIFT:   return EXEC_BIOP(>>, val1, val2);

      case TK_AND:      return EXEC_BIOP(&&, val1, val2);
      case TK_OR:       return EXEC_BIOP(||, val1, val2);

      case TK_BIT_AND:  return EXEC_BIOP(&,  val1, val2);
      case TK_BIT_OR:   return EXEC_BIOP(|,  val1, val2);
      case TK_BIT_XOR:  return EXEC_BIOP(^,  val1, val2);
      default: assert(0);
    }
    #undef EXE_BIOP
  }

  else if (is_unary_operator(op_idx)) {
    bad_expr_assert (tk_start == op_idx);
    bad_expr_assert (tk_start + 1 <= tk_end);
    switch (tokens[op_idx].type) {
      case TK_NEG: return (word_t) -eval(tk_start + 1, tk_end);
      case TK_POS: return (word_t) eval(tk_start + 1, tk_end);
      case TK_BIT_REVERSE: return (word_t) ~eval(tk_start + 1, tk_end);
      case TK_DEREF: {
        paddr_t paddr = (word_t) eval(tk_start + 1, tk_end);
        if (!in_storage(paddr)) {
          // printf("Address to read (" FMT_PADDR ") is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "]\n", paddr, PMEM_LEFT, PMEM_RIGHT);
          bad_expr_assert(0);
        }
        return (word_t) paddr_read(paddr, 4); // len = 4, read 4 byte (1 word);
      }
      default: bad_expr_assert(0);
    }
  }

  bad_expr_assert(0);
  return 0;
}

word_t expr(char *e, bool *success) {
  
  if (!make_token(e)) {
    printf("Failed to make tokens.\n");
    *success = false;
    return 0;
  }

  word_t val = 0;

  unsigned int tk_start = 0;
  unsigned int tk_end = nr_token - 1;
  assert(tk_end >= tk_start);

  if (setjmp(jpb) == 0) {
    val = eval(tk_start, tk_end);
  }
  else {
    printf("Invalid Expression!\n");
    *success = false;
    return 0;
  }

  *success = true;
  Log("\033[32m" "val=%u (0x%08x, %d)" "\033[0m", val, val, val);
  return val;
}


void test_expr() {
  #ifdef CONFIG_EXPR_TEST_ENABLE

  // #define CONFIG_EXPR_TEST_PATH "./tools/gen-expr/expr.txt"
  // #define CONFIG_EXPR_TEST_NUM 1000
  #define EXPR_TEST_GEN(n) "tools/gen-expr/build/gen-expr "\
  str(n) " > " CONFIG_EXPR_TEST_PATH " 2>tools/gen-expr/err.log"
  // str(n) " > " CONFIG_EXPR_TEST_PATH " 2>./tools/gen-expr/err.log"
  
  #ifdef CONFIG_EXPR_TEST_GEN_ENABLE
  // gen expr
  Log("running \033[31m" EXPR_TEST_GEN(CONFIG_EXPR_TEST_NUM) "\033[0m");
  int ret = system(EXPR_TEST_GEN(CONFIG_EXPR_TEST_NUM));
  if (!(WIFEXITED(ret) && WEXITSTATUS(ret) == 0)) {
    panic("Failed to generate test experessions!");
  }
  #endif

  // read expr
  FILE* fp; 
  if ((fp = fopen(CONFIG_EXPR_TEST_PATH, "r")) == NULL) {
    puts(CONFIG_EXPR_TEST_PATH);
    perror("Open file failed!");
  }

  char* line;
  if ((line = (char *)calloc(65536, sizeof(char))) == NULL) {
    panic("Failed to alloc memory!");
  }

  int i = 0;
  int pass = 0;
  while(fgets(line, 65536, fp)) { // not sizeof(line), line is a pointer
    line[strcspn(line, "\n")] = '\0';

    char *eq = strchr(line, '=');
    if (eq) {
        *eq = '\0'; 
        char *result_str = line;
        char *expr_str   = eq + 1;

        word_t result = (word_t) strtoul(result_str, NULL, 0);
        bool success = 0;
        word_t test_result = expr(expr_str, &success);
        assert(success);
        Log("test[%d] gt=%u, test=%u", i, result, test_result);
        if (result == test_result) {
          pass++;
        } else {
          Log("\033[31m" "test[%d] failed." "\033[0m", i);
        }
    }
    ++i;
  }
  if (pass == CONFIG_EXPR_TEST_NUM) {
    Log("\033[32m" "All %d expr-tests passed." "\033[0m", pass);
  }
  else {
    Log("\033[31m" "%d/%d expr-tests failed." "\033[0m", CONFIG_EXPR_TEST_NUM - pass, CONFIG_EXPR_TEST_NUM);
  }

  fclose(fp);
  free(line);
  #endif
}