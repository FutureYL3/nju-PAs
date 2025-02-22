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

#include <common.h>

#include "/home/yl/ics2022/nemu/src/monitor/sdb/sdb.h"

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

//   /* Start engine. */
//   engine_start();

//   return is_exit_status_bad();

  FILE *fp = fopen("/home/yl/ics2022/nemu/tools/gen-expr/input", "r");
  assert(fp != NULL);
  word_t expected = 0;
  char buf[65536 + 100] = {0};
  char exp[65536] = {0};
  int count = 0;
  while (fgets(buf, sizeof(buf), fp) != NULL) {
    if (sscanf(buf, "%u %[^\n]", &expected, exp) == 2) {
      printf("Number: %u, String: %s\n", expected, exp);
    }
    
    bool success = false;
    word_t actual = expr(exp, &success);
    if (success) {
      if (actual == expected) {
        count++;
        printf("Match!! expected is %u, actual is %u\n", expected, actual);
      } else {
        printf("Don't Match. expected is %u, actual is %u\n", expected, actual);
      }
    } else {
      printf("evaluation failed at %s\n", exp);
    }
  }
  printf("Total match: %d/%d\n", count, 9284);
  fclose(fp);
  return 0;
}
