#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>

uint32_t eval(int p, int q);
bool check_parentheses(int p, int q);
int find_dominant_operator(int p, int q);

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


#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )//计算元素个数
static regex_t re[NR_REGEX];//存储已经编译的正则表达式


/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
	char error_msg[128]; // 字符数组 用于存储错误信息的缓冲区
	int ret; // 存储 regcomp() 返回值，用于检查编译是否成功

	int i;
	for(i = 0; i < NR_REGEX; i ++) {
		ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);

		//错误处理 
		if(ret != 0) {
			regerror(ret, &re[i], error_msg, 128);//将 regcomp() 返回的错误码转换为可读的错误信息，并存储在 error_msg 中
			Assert(ret == 0, "regex compilation failed: %s\n%s", error_msg, rules[i].regex);
		}
	}
}

//定义token类型和数组Token
typedef struct token {
	int type;
	char str[32];
} Token;

Token tokens[32];
int nr_token;

uint32_t registers[] = {
    0x100, // eax
    0x200, // ebx
    0x300, // ecx
    0x400, // edx
    0x500, // esp
    0x600, // ebp
    0x700, // esi
    0x800, // edi
    0x900  // eip
};


//对每一个符号进行分类，再将各个类型存储在tokens[]数组中
static bool make_token(char *e) {
	int position = 0;
	int i;
	regmatch_t pmatch;
	
	nr_token = 0;//用于记录当前已经生成的 token 数量

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

//不能处理成token流，直接false
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
        printf("Bad expression\n");
        exit(1);
    } else if (p == q) {
        uint32_t num = 0;
        switch (tokens[p].type) {
            case NUM:
                sscanf(tokens[p].str, "%d", &num);
                return num;
            case HEXNUM:
                sscanf(tokens[p].str, "%x", &num);
                return num;
            case REGNAME:
                if (strcmp(tokens[p].str, "$eax") == 0) return registers[R_EAX];
                if (strcmp(tokens[p].str, "$ebx") == 0) return registers[R_EBX];
                if (strcmp(tokens[p].str, "$ecx") == 0) return registers[R_ECX];
                if (strcmp(tokens[p].str, "$edx") == 0) return registers[R_EDX];
                if (strcmp(tokens[p].str, "$esp") == 0) return registers[R_ESP];
                if (strcmp(tokens[p].str, "$ebp") == 0) return registers[R_EBP];
                if (strcmp(tokens[p].str, "$esi") == 0) return registers[R_ESI];
                if (strcmp(tokens[p].str, "$edi") == 0) return registers[R_EDI];
                
                printf("Unknown register: %s\n", tokens[p].str);
                exit(1);
            default:
                printf("Unexpected token type: %d\n", tokens[p].type);
                exit(1);
        }
    } else if (check_parentheses(p, q)) {
		// 如果括号平衡，则递归求值括号内的表达式
		return eval(p + 1, q - 1);
    } else {
        int op = find_dominant_operator(p, q);
        if (op == -1) {
            printf("No operator found\n");
            exit(1);
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
            case NOT: return !val2;
            default:
                printf("Unexpected operator: %d\n", tokens[op].type);
                exit(1);
        }
    }

    return 0;
}
// uint32_t eval(int p, int q) {
// 	if (p > q) {
// 		panic("Bad expression");
// 	}

// 	if (p == q) {
// 		switch (tokens[p].type) {
// 			case NUM: {
// 				uint32_t num;
// 				sscanf(tokens[p].str, "%d", &num);
// 				return num;
// 			}
// 			case HEXNUM: {
// 				uint32_t num;
// 				sscanf(tokens[p].str, "%x", &num);
// 				return num;
// 			}
// 			default:
// 				panic("Unexpected token type");
// 		}
// 	}

// 	if (check_parentheses(p, q)) {
// 		return eval(p + 1, q - 1);
// 	}

// 	int op = find_main_operator(p, q);

// 	if (tokens[op].type == NOT) {
// 		uint32_t val = eval(op + 1, q);
// 		return !val;
// 	}

// 	uint32_t val1 = eval(p, op - 1);
// 	uint32_t val2 = eval(op + 1, q);

	
// 	switch (tokens[op].type) {
// 		case '+': return val1 + val2;
// 		case '-': return val1 - val2;
// 		case '*': return val1 * val2;
// 		case '/': return val1 / val2;
// 		case EQ: return val1 == val2;
// 		case NOTEQUAL: return val1 != val2;
// 		case AND: return val1 && val2;
// 		case OR: return val1 || val2;
// 		case REGNAME: //需要在这里添加寄存器的情况，并且添加寄存器求值的函数
// 		default:
// 			panic("Unexpected operator");
// 	}

// 	return 0;
// }
bool check_parentheses(int p, int q) {
	int count = 0;
    int i;
	for (i = p; i <= q; i++) {
		if (tokens[i].type == '(') count++;
		if (tokens[i].type == ')') count--;
		if (count < 0) return false;  // 如果在此过程中出现右括号多于左括号，直接返回false
	}
	return count == 0; // 判断最终括号是否平衡
}

// bool check_parentheses(int p, int q) {
// 	if (tokens[p].type == '(' && tokens[q].type == ')') {
// 		int count = 0;
// 		int i;
// 		for (i = p; i <= q; i++) {
// 			if (tokens[i].type == '(') count++;
// 			if (tokens[i].type == ')') count--;
// 			if (count == 0 && i < q) return false;
// 		}
// 		return count == 0;
// 	}
// 	return false;
// }

int find_dominant_operator(int p, int q) {
    int op = -1;
    int level = 0;
    int lowest_priority = 8; // 设置为比任何运算符的优先级都高的值
    int i;
    for (i = p; i <= q; i++) {
        if (tokens[i].type == '(') {
            level++;
        } else if (tokens[i].type == ')') {
            level--;
        } else if (level == 0) {
            int current_priority = -1;
            switch (tokens[i].type) {
                case OR: current_priority = 1; break;
                case AND: current_priority = 2; break;
                case EQ:
                case NOTEQUAL: current_priority = 3; break;
                case '+':
                case '-': current_priority = 4; break;
                case '*':
                case '/': current_priority = 5; break;
                case NOT: current_priority = 6; break;
                default: break;
            }
            // 找到优先级最低的运算符
            if (current_priority < lowest_priority && current_priority != -1) {
                lowest_priority = current_priority;
                op = i;
            }
        }
    }

    return op;
}

// uint32_t eval(int p, int q) {
//     if (p > q) {
//         printf("Bad expression\n");
//         exit(1);
//     } else if (p == q) {
//         uint32_t num = 0;
//         switch (tokens[p].type) {
//             case NUM:
//                 sscanf(tokens[p].str, "%d", &num);
//                 return num;
//             case HEXNUM:
//                 sscanf(tokens[p].str, "%x", &num);
//                 return num;
//             default:
//                 printf("Unexpected token type: %d\n", tokens[p].type);
//                 exit(1);
//         }
//     } else if (check_parentheses(p, q)) {
//         return eval(p + 1, q - 1);
//     } else {
//         int op = find_dominant_operator(p, q);
//         if (op == -1) {
//             printf("No operator found\n");
//             exit(1);
//         }

//         uint32_t val1 = eval(p, op - 1);
//         uint32_t val2 = eval(op + 1, q);

//         switch (tokens[op].type) {
//             case '+': return val1 + val2;
//             case '-': return val1 - val2;
//             case '*': return val1 * val2;
//             case '/': return val1 / val2;
//             case EQ: return val1 == val2;
//             case NOTEQUAL: return val1 != val2;
//             case AND: return val1 && val2;
//             case OR: return val1 || val2;
//             case NOT: return !val2;
//             default:
//                 printf("Unexpected operator: %d\n", tokens[op].type);
//                 exit(1);
//         }
//     }

//     return 0;
// }

// uint32_t expr(char *e, bool *success) {
//     if (!make_token(e)) {
//         *success = false;
//         return 0;
//     }

//     *success = true;
//     return eval(0, nr_token - 1);
// }

// int main() {
//     init_regex();

//     char expression[256];
//     bool success;

//     printf("Enter an expression: ");
//     if (fgets(expression, 256, stdin) == NULL) {
//     printf("读取输入时发生错误。\n");
//     return 1;
//     }

//     expression[strcspn(expression, "\n")] = '\0';

//     uint32_t result = expr(expression, &success);

//     if (success) {
//         printf("Result: %u\n", result);
//     } else {
//         printf("Invalid expression.\n");
//     }

//     return 0;
// }
