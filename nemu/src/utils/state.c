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

#include <utils.h>
#define MAX_IRINGBUF_SIZE 20
extern char iringbuf[MAX_IRINGBUF_SIZE][100];
extern int iringbuf_cur_next;

NEMUState nemu_state = { .state = NEMU_STOP };

int is_exit_status_bad() {
	/* Print iringbuf content if program ends because of  exception */
	if (nemu_state.halt_ret != 0) {
		int cur = iringbuf_cur_next == 0 ? MAX_IRINGBUF_SIZE - 1 : iringbuf_cur_next - 1;
		iringbuf[cur][0] = '-'; iringbuf[cur][1] = '-'; iringbuf[cur][2] = '>';
		printf("\niringbuf trace:\n");
		for (int i = 0; i < MAX_IRINGBUF_SIZE; ++ i)  printf("%s", iringbuf[i]);
	}

  int good = (nemu_state.state == NEMU_END && nemu_state.halt_ret == 0) ||
    (nemu_state.state == NEMU_QUIT);
  return !good;
}
