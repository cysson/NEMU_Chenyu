#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

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
