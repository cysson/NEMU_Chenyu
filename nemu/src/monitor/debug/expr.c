#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

uint32_t eval(int p, int q);
bool check_parentheses(int p, int q);
int find_main_operator(int p, int q);

enum {
	NOTYPE = 256, EQ, NOTEQUAL, NUM, HEXNUM, REGNAME, AND, OR, NOT

	/* TODO: Add more token types */

};

static struct rule {
	char *regex;
	int token_type;
} rules[] = {

	/* TODO: Add more rules.
	 * Pay attention to the precedence level of different rules.
	 */

	{" +",	NOTYPE},				// spaces

	{"\\+", '+'},					// plus
	{"\\-", '-'},					// minus
	{"\\*", '*'},					// multiply
	{"\\/", '/'},					// divide
	{"\\(", '('},					// left parenthesis
	{"\\)", ')'},					// right parenthesis
	{"==", EQ},						// equal
	{"!=", NOTEQUAL},				// not equal

	{"[0-9]+", NUM},				// decimal number
	{"0x[0-9a-f]+", HEXNUM},		// hexadecimal number
	{"\\$[a-z]{2,3}", REGNAME},		// register name, fixed: no space in {2,3}

	{"&&", AND},					// logical and
	{"\\|\\|", OR},					// logical or
	{"!", NOT}						// logical not
};


#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	int i;
	char error_msg[128];
	int ret;

	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;

	while(e[position] != '\0') {
		/* Try all rules one by one. */
		for(i = 0; i < NR_REGEX; i ++) {
			if(regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
				char *substr_start = e + position;
				int substr_len = pmatch.rm_eo;

				Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s", 
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);
				
				// 移动位置
				position += substr_len;

				// 如果匹配的是空格，跳过
				if(rules[i].token_type == NOTYPE) {
					break;
				}

				// 保存匹配到的 token
				tokens[nr_token].type = rules[i].token_type;
				memset(tokens[nr_token].str, 0, sizeof(tokens[nr_token].str));
				strncpy(tokens[nr_token].str, substr_start, substr_len);
				nr_token++;

				break;
			}
		}

		if(i == NR_REGEX) {
			printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
			return false;
		}
	}

	return true; 
}


uint32_t expr(char *e, bool *success) {
	if (!make_token(e)) {
		*success = false;
		return 0;
	}

	// 调用 eval 函数来计算表达式的值
	*success = true;
	return eval(0, nr_token - 1);
}

uint32_t eval(int p, int q) {
	if (p > q) {
		panic("Bad expression");
	}

	if (p == q) {
		switch (tokens[p].type) {
			case NUM: {
				uint32_t num;
				sscanf(tokens[p].str, "%d", &num);
				return num;
			}
			case HEXNUM: {
				uint32_t num;
				sscanf(tokens[p].str, "%x", &num);
				return num;
			}
			default:
				panic("Unexpected token type");
		}
	}

	if (check_parentheses(p, q)) {
		return eval(p + 1, q - 1);
	}

	int op = find_main_operator(p, q);

	if (tokens[op].type == NOT) {
		uint32_t val = eval(op + 1, q);
		return !val;
	}

	uint32_t val1 = eval(p, op - 1);
	uint32_t val2 = eval(op + 1, q);

	switch (tokens[op].type) {
		case '+': return val1 + val2;
		case '-': return val1 - val2;
		case '*': return val1 * val2;
		case '/': return val1 / val2;
		case EQ: return val1 == val2;
		case NOTEQUAL: return val1 != val2;
		case AND: return val1 && val2;
		case OR: return val1 || val2;
		default:
			panic("Unexpected operator");
	}

	return 0;
}

bool check_parentheses(int p, int q) {
	if (tokens[p].type == '(' && tokens[q].type == ')') {
		int count = 0;
		int i;
		for (i = p; i <= q; i++) {
			if (tokens[i].type == '(') count++;
			if (tokens[i].type == ')') count--;
			if (count == 0 && i < q) return false;
		}
		return count == 0;
	}
	return false;
}

int find_main_operator(int p, int q) {
	int i;
	int level = 0;
	int op = -1;
	int priority = -1;

	for (i = p; i <= q; i++) {
		if (tokens[i].type == '(') {
			level++;
		} else if (tokens[i].type == ')') {
			level--;
		} else if (level == 0) {
			int cur_priority = -1;
			switch (tokens[i].type) {
				case OR: cur_priority = 1; break;
				case AND: cur_priority = 2; break;
				case EQ:
				case NOTEQUAL: cur_priority = 3; break;
				case '+':
				case '-': cur_priority = 4; break;
				case '*':
				case '/': cur_priority = 5; break;
				case NOT: cur_priority = 6; break;
				default: break;
			}
			if (cur_priority > priority) {
				priority = cur_priority;
				op = i;
			}
		}
	}

	return op;
}


