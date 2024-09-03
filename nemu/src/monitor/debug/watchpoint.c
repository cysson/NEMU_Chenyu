#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include "nemu.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;
int test_change();

    
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

int test_change() {
    WP *p = head;  // Pointer to the current watchpoint
    bool success = false;  // Used to indicate whether the expression evaluation succeeded
    int changed = 0;       // Used to record if any watchpoint has changed

    // Traverse through all watchpoints
    while (p != NULL) {
        // Evaluate the new value of the current watchpoint expression
        uint32_t new_value = expr(p->exp, &success);

        // If the expression evaluation failed, output an error message
        if (!success) {
            printf("Failed to evaluate expression: %s\n", p->exp);
            return 0;  // If evaluation fails, return immediately
        }

        // If the expression value has changed
        if (new_value != p->value) {
            // Output the watchpoint details, including the watchpoint number, expression, old value, and new value
            printf("Watchpoint %d: %s value changed from %d to %d\n", p->NO, p->exp, p->value, new_value);
            
            // Update the watchpoint's value to the new value
            p->value = new_value;
            
            // Indicate that a watchpoint value has changed
            changed = 1;
        }

        // Move to the next watchpoint
        p = p->next;
    }

    return changed;  // Return 1 if any value changed, 0 if no values changed
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