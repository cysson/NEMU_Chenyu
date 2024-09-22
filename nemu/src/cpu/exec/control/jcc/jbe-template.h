#include "cpu/exec/template-start.h"

#define instr jbe

static void do_execute() {
	print_asm("jbe %x", cpu.eip + 1 + DATA_BYTE + op_src->val);
	if(cpu.eflags.CF == 1 || cpu.eflags.ZF == 1) cpu.eip += op_src->val;
}

make_instr_helper(si)

#include "cpu/exec/template-end.h"