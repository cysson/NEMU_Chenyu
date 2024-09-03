#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
bool check_wp ();

    
void init_wp_pool() {
	int i;
	for(i = 0; i < NR_WP; i ++) {
		wp_pool[i].NO = i;
		wp_pool[i].next = &wp_pool[i + 1];
	}
	wp_pool[NR_WP - 1].next = NULL;

	head = NULL;
	free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */
WP* new_wp(char * exp){
	assert(free_ != NULL);
	WP *temp = free_;
	free_ = free_->next;
	temp->next = NULL;

	bool success = false;
	strcpy(temp->exp, exp);
	temp->value = expr(temp->exp, &success);
	assert(success);

	if(head==NULL)
		head = temp;
	else{
		WP *p = head;
		while(p->next)
			p = p->next;
		p->next = temp;
	}
	return temp;
}


int free_wp(WP *wp) {
	if(wp == NULL) {
	    printf("Input something!\n");
	    return 0;
	}
	if(wp == head) {
	    head = head->next;
	}
	else {
	    WP *p = head;
	    while(p->next != wp) {
	        p = p->next;
	        if(p == NULL) {
	            return 0; // wp not found
	        }
	    }
	    p->next = wp->next;
	}
	wp->next = free_; // Return the wp to the free pool
	free_ = wp;
	return 1;
}

bool check_wp(){
    WP* wp;
    wp = head;
    bool suc, key;
    key = true;
    while(wp != NULL){
        int val = expr(wp->exp, &suc);  // 使用 wp->exp
        if(!suc) assert(1);
        if(wp->value != val){  // 使用 wp->value
            key = false;
            printf("Hint breakpoint %d at address 0x%08x\n", wp->NO, cpu.eip);
            printf("Watchpoint %d: %s\n", wp->NO, wp->exp);  // 使用 wp->exp
            printf("Old value = %d\n", wp->value);  // 使用 wp->value
            printf("New value = %d\n", val);
            wp->value = val;  // 更新 wp->value
        }
        wp = wp->next;
    }
    
    return key;
}


int delete_wp(int num) {
    WP *p = head;
    while(p->next && p->next->NO != num) {
        p = p->next;
    }
    if (p->next && p->next->NO == num) {
        WP *to_delete = p->next;
        p->next = to_delete->next;
        free_wp(to_delete);
        return 1;  // 返回 1 表示成功删除
    } else {
        printf("Unexpected number, delete failed\n");
        return 0;  // 返回 0 表示删除失败
    }
}


void print_wp() {
    WP *p = head;
    while (p != NULL) {
        printf("Watchpoint %d: %s = %d\n", p->NO, p->exp, p->value);
        p = p->next;
    }
}