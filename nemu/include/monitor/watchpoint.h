#ifndef __WATCHPOINT_H__
#define __WATCHPOINT_H__

#include "common.h"

typedef struct watchpoint {
	int NO;
	struct watchpoint *next;
	char exp[32];
	uint32_t value;
	/* TODO: Add more members if necessary */


} WP;

WP* new_wp();   // 创建一个新的监视点
int free_wp();     // 释放一个监视点
bool check_wp();       // 检测监视点值是否变化
void print_wp();
int delete_wp(int num);
#endif
