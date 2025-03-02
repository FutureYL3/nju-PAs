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
/* for memory read */
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DEC, TK_LPAR, TK_RPAR, TK_HEX, TK_REG, TK_NOTEQ, TK_AND, TK_DEREF
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
	{"0x[0-9a-fA-F]+", TK_HEX}, // hexadecimal number
	{"\\$(\\$0|ra|sp|gp|tp|t[0-6]|s[0-9]|a[0-7]|s10|s11)", TK_REG}, // register
	{"[0-9]+", TK_DEC},     // decimal number
	{"\\(", TK_LPAR},     // left parentheses
	{"\\)", TK_RPAR},     // right parentheses
	{"\\*", '*'},         // times
  {"/", '/'},           // divide
  {"\\+", '+'},         // plus
	{"-", '-'},           // minus
  {"==", TK_EQ},        // equal
	{"!=", TK_NOTEQ},     // not equal
	{"&&", TK_AND},       // and
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

static Token tokens[65536] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  // for (int i = 0; i < NR_REGEX; ++ i) {
  //   printf("rule[%d] with regex %s type %d\n", i, rules[i].regex, rules[i].token_type);
  // }

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
					case TK_HEX:
						// if out of buffer, panic and ends the debugger
						if (substr_len > 31 + 2) { // "0x" is not counted
							char *hex = (char *)calloc(substr_len, sizeof(char));
							for (int j = 0; j < substr_len; ++ j) {
								hex[j] = *(substr_start + j);
							}
							/* TODO */
							panic("hex %s is larger than 32 bits\n", hex);
						}

						tokens[nr_token].type = TK_HEX;
						for (int j = 2; j < substr_len; ++ j) { // ignore "0x"
							tokens[nr_token].str[j-2] = *(substr_start + j);
						}
            tokens[nr_token].str[substr_len-2] = '\0';
						nr_token++;
						break;	
					case TK_REG:
						tokens[nr_token].type = TK_REG;
						for (int j = 1; j < substr_len; ++ j) { // ignore "$"
							tokens[nr_token].str[j-1] = *(substr_start + j);
						}
						tokens[nr_token].str[substr_len-1] = '\0';
						nr_token++;
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
					case TK_NOTEQ:
						tokens[nr_token++].type = TK_NOTEQ;
						break;
					case TK_AND:
						tokens[nr_token++].type = TK_AND;
						break;	
					default: panic("unknown token type: %d\n", rules[i].token_type);
        }

				char *val = (char *)calloc(substr_len, sizeof(char));
				for (int j = 0; j < substr_len; ++ j) {
					val[j] = *(substr_start + j);
				}
			  // printf("find token %s, type %d\n", val, rules[i].token_type);	

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

	// printf("make token finished\n");
	// for (int j = 0; j < nr_token; ++ j) {
	// 	printf("token[%d] has type %d, str %s\n", j, tokens[j].type, tokens[j].str);
	// }

  return true;
}

static word_t eval(int p, int q, bool *success);

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

	/* check for dereference */
	for (int i = 0; i < nr_token; ++ i) {
		if (tokens[i].type == '*' && (i == 0 || tokens[i-1].type == TK_LPAR || tokens[i-1].type == '*' || tokens[i-1].type == '/' || tokens[i-1].type == '+' || tokens[i-1].type == '-' || tokens[i-1].type == TK_EQ || tokens[i-1].type == TK_NOTEQ || tokens[i-1].type == TK_AND)) {
			tokens[i].type = TK_DEREF;
		}
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
		// TODO: add hex and register eval
    /* Single token.
		 * For now this token should be a decimal/hexadecimal/register.
		 * Return the value of the token.
		 */
		if (tokens[p].type != TK_DEC && tokens[p].type != TK_HEX && tokens[p].type != TK_REG) {
			*success = false;
			fprintf(stderr, "token[%d] is not a decimal/hexadecimal/register\n", p);
			return 0;
		}

		word_t val = 0;
		int mul = 1;
		int nr_str = 0;

		switch (tokens[p].type) {
			case TK_DEC:
	  		while (tokens[p].str[nr_str] != 0)  nr_str++;
    		// printf("nr_str of %s is %d\n", tokens[p].str, nr_str);

				for (int i = nr_str - 1; i >= 0; -- i) {
					word_t pre_val = val;
					val += (tokens[p].str[i] - 48) * mul;
					if ((tokens[p].str[i] - 48) * mul > val || pre_val > val) {
						printf("Warning: token[%d] overflowed\n", p);
					}
					mul *= 10;
				}
				break;	
			case TK_REG:
				bool reg_success = false;
				// printf("tokens[p].str = %s\n", tokens[p].str);
				val = isa_reg_str2val(tokens[p].str, &reg_success);
				if (!reg_success) {
					*success = false;
					fprintf(stderr, "evaluate the value of register %s at token[%d] failed\n", tokens[p].str, p);
					return 0;
				}
				break;
			case TK_HEX:
				while (tokens[p].str[nr_str] != 0)  nr_str++;

				for (int i = nr_str - 1; i >= 0; -- i) {
					word_t pre_val = val;
					int another_mul = 0;
					switch (tokens[p].str[i]) {
						case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
							val += (tokens[p].str[i] - 48) * mul;
							another_mul = tokens[p].str[i] - 48;
							break;
						case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
							val += (tokens[p].str[i] - 87) * mul;
							another_mul = tokens[p].str[i] - 87;
							break;
						case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
							val += (tokens[p].str[i] - 55) * mul;
							another_mul = tokens[p].str[i] - 55;
							break;
					}	
					if (another_mul * mul > val || pre_val > val) {
						printf("Warning: token[%d] overflowed\n", p);
					}
					mul *= 16;
				}
				break;
		}
		// printf("hit number, index is %d, value is %u\n", p, val);
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
		// TODO: add == != && and *, treat * as special
		if (!can_split) {
			fprintf(stderr, "invalid can't split expr from token %d to %d\n", p, q);
			*success = false;
			return 0;
		}

		if (tokens[p].type == TK_DEREF) {
			bool eval_success = false;
			word_t tmp = eval(p+1, q, &eval_success);

			if (!eval_success) {
				fprintf(stderr, "failed to evaluate expr from token %d to %d\n", p+1, q);
				*success = false;
				return 0;
			}

			vaddr_t addr = (vaddr_t) tmp;
			word_t val = vaddr_read(addr, 4);

			*success = true;
			return val;
		}
		else {
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
				case TK_EQ: *success = true; return val1 == val2 ? 1 : 0;
				case TK_NOTEQ: *success = true; return val1 != val2 ? 1 : 0;
				case TK_AND: *success = true; return (val1 > 0 && val2 > 0) ? 1 : 0;
      	default: *success = false; assert(0);
   		}
		}
	}		
}

static int get_precedence_level(int i) {
	/* larger the number, more precident the level */
	// TODO: consider when add == != and &&
	switch (tokens[i].type) {
		case TK_AND:
			return 0;
		case TK_EQ: case TK_NOTEQ:
			return 1;
		case '+': case '-':
			return 2;
		case '*': case '/':
			return 3;
	}

	return 0;
}

static int find_main_op_position(int p, int q) {
	// TODO: consider when add == != and &&
	int op = -1, nr_lpar = 0;
	for (int i = p; i <= q; ++ i) {
		switch (tokens[i].type) {
			case TK_LPAR:
				nr_lpar++;
				break;
			case TK_RPAR:
				nr_lpar--;
				break;
			case '+': case '-': case '*': case '/': case TK_EQ: case TK_NOTEQ: case TK_AND:
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


