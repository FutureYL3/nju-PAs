/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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
#include <sys/wait.h>  // for examine child thread exit status
#include <signal.h>    // for SIGFPE

// this should be enough
static int len = 0;
static char buf[65536] = {0};
static char code_buf[65536 + 128] = {0}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

/* Generate random number ranges from 0 to n-1 */
static int choose(int n) {
  return rand() % n;
}

static void gen_num() {
  /* The random number ranges from 1 to 9 */
  buf[len++] = (char) (choose(9) + 1 + 48); // avoid 0-start number, generate number from 1-9
  // buf[len++] = (char) (choose(10) + 48); 
  buf[len++] = 'U'; // add U appendix to ensure unsigned operation
}

static void gen(char c) {
  buf[len++] = c;
}

static void gen_rand_op() {
  switch (choose(4)) {
  case 0:
    buf[len++] = '+';
    break;
  case 1:
    buf[len++] = '-';
    break;
  case 2:
    buf[len++] = '*';
    break;
  default:
    buf[len++] = '/';
    break;
  }
}

static void gen_rand_expr() {
  switch (choose(4)) {
    case 0: gen_num(); break;
    case 1: gen('('); gen_rand_expr(); gen(')'); break;
    case 2: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
    default: gen(' '); gen_rand_expr(); gen(' '); break; // randomly generate space
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 100;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    /* reset the buf and codebuf */
    memset(buf, 0, sizeof(buf));
    memset(code_buf, 0, sizeof(code_buf));
    len = 0;

    gen_rand_expr();
    buf[len] = '\0';
    printf("Expr generation round %d done, %s was generated\n", i+1, buf);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc -Werror /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr 2>/dev/null", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);

    // get child thread exit status
    int status = pclose(fp);

    // check for division by zero (SIGFPE)
    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGFPE) {
      /* do nothing */
      continue;
    } else {
      printf("%u %s\n", result, buf);
    }
  }
  return 0;
}
