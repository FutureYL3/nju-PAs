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

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
/* for debug macro usage */
#include <debug.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DEC, TK_LPAR, TK_RPAR
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
	{"\\d+", TK_DEC},     // decimal number
	{"\\(", TK_LPAR},     // left parentheses
	{"\\)", TK_RPAR},     // right parentheses
	{"\\*", '*'},         // times
  {"/", '/'},           // divide
  {"\\+", '+'},         // plus
	{"-", '-'},           // minus
  {"==", TK_EQ},        // equal
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

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

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  for (int i = 0; i < NR_REGEX; ++ i) {
    printf("rule[%d] with regex %s type %d\n", i, rules[i].regex, rules[i].token_type);
  }

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
					case TK_NOTYPE:
						break;
					case TK_DEC:
						// if out of buffer, panic and ends the debugger
						if (substr_len > 31) {
							char *number = (char *)calloc(substr_len, sizeof(char));
							for (int j = 0; j < substr_len; ++ j) {
								number[j] = *(substr_start + j);
							}
							/* TODO: throw error message and and discard this time's expr eval, do not panic for better user experience */
							panic("number %s is larger that 32 bits\n", number);
						}
						
						tokens[nr_token].type = TK_DEC;
						for (int j = 0; j < substr_len; ++ j) {
							tokens[nr_token].str[j] = *(substr_start + j);
						}
            tokens[nr_token].str[substr_len] = '\0';
						nr_token++;
						break;
					case TK_LPAR:
						tokens[nr_token++].type = TK_LPAR;
						break;
					case TK_RPAR:
						tokens[nr_token++].type = TK_RPAR;
						break;
					case '*':
						tokens[nr_token++].type = '*';
						break;
					case '/':
						tokens[nr_token++].type = '/';
						break;
					case '+':
						tokens[nr_token++].type = '+';
						break;
					case '-':
						tokens[nr_token++].type = '-';
						break;
					case TK_EQ:
						tokens[nr_token++].type = TK_EQ;
						break;
					default: panic("unknown token type: %d\n", rules[i].token_type);
        }

				char *val = (char *)calloc(substr_len, sizeof(char));
				for (int j = 0; j < substr_len; ++ j) {
					val[j] = *(substr_start + j);
				}
			  printf("find token %s, type %d\n", val, rules[i].token_type);	

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

	printf("make token finished\n");
	for (int j = 0; j < nr_token; ++ j) {
		printf("token[%d] has type %d, str %s\n", j, tokens[j].type, tokens[j].str);
	}

  return true;
}

static word_t eval(int p, int q, bool *success);

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
	bool eval_success;
  word_t val = eval(0, nr_token - 1, &eval_success);
	if (!eval_success) {
		*success = false;
		return 0;
	}	

	*success = true;
	return val;	
}

static int get_precedence_level(int i);
static int find_main_op_position(int p, int q);
static bool check_parentheses(int p, int q, bool *can_split);

static word_t eval(int p, int q, bool *success) {
	bool can_split = false;
	if (p > q) {
    /* Bad expression */
		fprintf(stderr, "%d as token start is larger that %d as end\n", p, q);
		*success = false;
		return 0;
 	}
  else if (p == q) {
    /* Single token.
		 * For now this token should be a number.
		 * Return the value of the number.
		 */
		if (tokens[p].type != TK_DEC) {
			*success = false;
			fprintf(stderr, "token[%d] is not a number\n", p);
			return 0;
		}

		word_t val = 0;
		int mul = 1;
	  int nr_str = 0;
	  while (tokens[p].str[nr_str] != 0)  nr_str++;

		for (int i = nr_str - 1; i >= 0; ++ i) {
			val += (tokens[p].str[i] - 48) * mul;
			mul *= 10;
		}
		
		printf("hit number, index is %d, value is %u\n", p, val);
		*success = true;
		return val;		
  }
  else if (check_parentheses(p, q, &can_split) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
		 * If that is the case, just throw away the parentheses.
		 */
	  return eval(p + 1, q - 1, success);
 	}
  else {
		if (!can_split) {
			fprintf(stderr, "invalid can't split expr from token %d to %d\n", p, q);
			*success = false;
			return 0;
		}

    int op = find_main_op_position(p, q);
		bool success1, success2;
    word_t val1 = eval(p, op - 1, &success1);
    word_t val2 = eval(op + 1, q, &success2);

		if (!success1 || !success2) {
			*success = false;
			return 0;
		}

		*success = true;
    switch (tokens[op].type) {
      case '+': *success = true; return val1 + val2;
      case '-': *success = true; return val1 - val2; 
      case '*': *success = true; return val1 * val2; 
      case '/': *success = true; return val1 / val2; 
      default: *success = false; assert(0);
    }
	}		
}

static int get_precedence_level(int i) {
	switch (tokens[i].type) {
		case '+': case '-':
			return 1;
		case '*': case '/':
			return 2;
	}

	return 0;
}

static int find_main_op_position(int p, int q) {
	int op = -1, nr_lpar = 0;
	for (int i = p; i <= q; ++ i) {
		switch (tokens[i].type) {
			case TK_LPAR:
				nr_lpar++;
				break;
			case TK_RPAR:
				nr_lpar--;
				break;
			case '+':
			case '-':
			case '*':
			case '/':
				if (nr_lpar > 0)  continue;
				if (op == -1) {
					op = i;
					break;
				}

				int cur_precedence_level = get_precedence_level(i);
				int op_precedence_level = get_precedence_level(op);
				if (cur_precedence_level < op_precedence_level) {
					op = i;
					break;
				}
				else if (cur_precedence_level > op_precedence_level) {
					/* do nothing */
				}
				else {
					op = i;
					break;
				}
		}
	}
	return op;
}

static bool check_parentheses(int p, int q, bool *can_split) {
	int nr_lpar = 0, zero_times = 0;
	*can_split = false; // give a default value
	for (int i = p; i <= q; ++ i) {
		if (tokens[i].type == TK_LPAR)  nr_lpar++;
		else if (tokens[i].type == TK_RPAR)  nr_lpar--;
		assert(nr_lpar >= 0);
		if (nr_lpar == 0)  zero_times++;
	}
	if (nr_lpar == 0) {
	  if (zero_times == 1)  return true;
		else  *can_split = true;
	}	

	return false;
}


