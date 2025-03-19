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
#include "sdb.h"
/* for strdup */
#include <string.h>

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
	char *EXPR;
	word_t prev_val;

} WP;

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

int new_wp(char *EXPR);
bool free_wp(int NO);

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
  }

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
// return the NO of allocated watchpoint, or -1 if failed
int new_wp(char *EXPR) {
	/* free_ equaling NULL indicates no more watchpoints available */
	if (free_ == NULL)  panic("No more watchpoints can be allocated\n");
	// manipulate linkedlist free_
	WP *new_one = free_;
	free_ = free_->next;
	new_one->next = NULL;

	// manipulate linkedlist head
	if (head == NULL)  head = new_one;
	else {
		WP *cur = head;
		while (cur->next != NULL)  cur = cur->next;
		cur->next = new_one;
	}

	// init new_one
	new_one->EXPR = strdup(EXPR);
	if (new_one->EXPR == NULL) {
		panic("Allocate memory for %s of watchpoint NO.%d failed\n", EXPR, new_one->NO);
	}

	bool expr_success = false;
	new_one->prev_val = expr(EXPR, &expr_success);
	if (!expr_success) {
		fprintf(stderr, "Warning: Failed to evaluate %s, make sure it's a valid expression\n", EXPR);
		free_wp(new_one->NO);
		return -1;
	}
	printf("added a new watchpoint with NO.%d and EXPR %s\n", new_one->NO, new_one->EXPR);
	return new_one->NO;
}	

// return whether the operation is success
bool free_wp(int NO) {
	if (head == NULL) {
		fprintf(stderr, "No watchpoints can be free, no watchpoint in head linkedlist\n");
		return false;
	}

	// manipulate linkedlist head
	WP *prev = NULL, *cur = head;
	while (cur != NULL && cur->NO != NO) {
		prev = cur;
		cur = cur->next;
	}
	if (cur == NULL) {
		fprintf(stderr, "Can't find watchpoint NO.%d, check your parameter\n", NO);	
		return false;
	}
	WP *free_one = NULL;
	if (prev == NULL) {
		free_one = head;
		head = head->next;
		free_one->next = NULL;
	}
	else {
		free_one = cur;
		prev->next = cur->next;
		free_one->next = NULL;
	}	

	// manipulate free_ linkedlist
	if (free_ == NULL)  free_ = free_one;
	else {
		WP *cur = free_;
		while (cur->next != NULL)  cur = cur->next;
		cur->next = free_one;
	}
	free_one->EXPR = NULL;

	return true;
}

void check_for_wp_change() {
	/* no watchpoint to be checked */
	if (head == NULL)  return;
	WP *cur = head;
	while (cur != NULL) {
		printf("check for watchpoint NO.%d EXPR %s\n", cur->NO, cur->EXPR);
		bool expr_success = false;
		word_t cur_val = expr(cur->EXPR, &expr_success);
		if (!expr_success) {
			fprintf(stderr, "Failed to evaluate EXPR %s at watchpoint NO.%d\n", cur->EXPR, cur->NO);
			cur = cur->next;
			continue;
		}

		// scan and print for all watchpoint changes 
		if (cur_val != cur->prev_val) {
			nemu_state.state = NEMU_STOP;
			printf("watchpoint change detected: watchpoint NO.%d with EXPR %s\nwith previous value %u(""0x%08" PRIx32 ") and current value %u(""0x%08" PRIx32 ")\n", cur->NO, cur->EXPR, cur->prev_val, cur->prev_val, cur_val, cur_val);
			cur->prev_val = cur_val;
		}
		cur = cur->next;
		// no break for all change detection
	}
}

void wp_display() {
	printf("NO\tprev value\t\t\tEXPR\n");
	WP *cur = head;
	while (cur != NULL) {
		printf("%d\t%u(""0x%08" PRIx32 ")\t\t\t%s\n", cur->NO, cur->prev_val, cur->prev_val, cur->EXPR);
		cur = cur->next;
	}
}

