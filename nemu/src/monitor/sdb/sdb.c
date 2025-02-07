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

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <utils.h>
/* to use strto?, need stdlib.h; to use uint?_t, need stdint.h. all included in common.h */
#include <common.h>
/* for trouble shooting */
#include <errno.h>
#include <limits.h>
/* for cmd_x to use vaddr_read */
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "usage: si [N]. Let the program pause after executing N instructions in a single step, when N is not given, the default is 1", cmd_si },
	{ "info", "usage: info (r,w). Print register or watchpoint status", cmd_info },
  { "x", "usage: x N EXPR. Evaluate the expression EXPR, use the result as the starting memory address, and output N sequential 4 bytes in hexadecimal.", cmd_x },
	
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  /* extract the first argument, ignore the remaining */
  char *arg = strtok(NULL, " ");
  uint64_t N;
	
  if (arg == NULL) {
    /* no argument given, N be the default 1 */
    N = 1; 
  }
  else {
		/* chech for negative number */
	  if (arg[0] == '-') {
			fprintf(stderr, "Warning: Negative number not allowed, use c(ontinue) instead\n");
			return 0;
		}
 	
    /* parse the argument to unsigned long long and convert it to uint64_t */
	  char *endptr;
		errno = 0; // reset the errno
		N = strtoull(arg, &endptr, 10); // 10 for decimal

		// error handling
		if (errno == ERANGE) {
			fprintf(stderr, "Warning: Overflow or underflow in conversion\n");
			/* return 0 for not ending the sdb_mainloop, but instead show the error message */
			return 0;
		}	
		
		if (endptr == arg || *endptr != '\0') {
			fprintf(stderr, "Warning: Invalid input string \"%s\"\n", arg);
			/* return 0 for not ending the sdb_mainloop, but instead show the error message */
			return 0;
		}
  }

	cpu_exec(N);
	return 0;
}

static int cmd_info(char *args) {
	/* extract the first argument, ignore the remaining */
	char *arg = strtok(NULL, " ");

	if (arg == NULL) {
		fprintf(stderr, "Warning: Please specify the SUBCMD(r(egister) or w(atchpoint))\n");
		return 0;
	}

	if (strcmp(arg, "r") == 0) {
		isa_reg_display();
		return 0;
	}
	else if (strcmp(arg, "w") == 0) {
		/* TODO: print watchpoint status */
		
		return 0;
	}

	fprintf(stderr, "Warning: Unknown SUBCMD: \"%s\"\n", arg);
	return 0;
}

#define LEN 4
static int cmd_x(char *args) {
	/* extract the first and the second argument, ignore the remaining */
	char *arg1 = strtok(NULL, " ");
	char *arg2 = strtok(NULL, " ");

	if (arg1 == NULL) {
		fprintf(stderr, "Warning: Missing argument N\n");
		return 0;
	}
	if (arg2 == NULL) {
		fprintf(stderr, "Warning: Missing argument EXPR\n");
		return 0;
	}

	/* parse the first argument to int */
  char *endptr;
	errno = 0; // reset errno
	// the max N is 33554432, by CONFIG_MSIZE / 4
	long tmp = strtol(arg1, &endptr, 10); // 10 for decimal
	
	// arg1 error handling
	if (errno != 0 || *endptr != '\0' || tmp < INT_MIN || tmp > INT_MAX) {
    fprintf(stderr, "Warning: Invalid number N(out of int bound or contains non-numeric character)\n");
    return 0;
  }	
	int N = (int)tmp;

	/* as for now, treat the second argument as hexadecimal value */
  /* parse the second argument to vaddr_t */
	errno = 0; // reset errno
	unsigned long tmp2 = strtoul(arg2, &endptr, 0); // 0 for hexadecimal
	if (errno != 0 || *endptr != '\0' || tmp2 > UINT32_MAX) {
    fprintf(stderr, "Warning: Invalid uint32_t (hex) input \"%s\"(out of word_t bound or contains invalid character\n", arg2);
    return 0;
  }	
	vaddr_t addr = (vaddr_t)tmp2;

	/* scan N 4-byte memory began at addr */
	printf("%s:", arg2);
	for (int i = 0; i < N; ++ i) {
		word_t val = vaddr_read(addr, LEN);
		addr += LEN;
		printf(" 0x%08" PRIx32, val);
	}
	printf("\n");
	return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
