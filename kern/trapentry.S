/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
	TRAPHANDLER_NOEC(handler0, T_DIVIDE)
	TRAPHANDLER_NOEC(handler1, T_DEBUG);
	TRAPHANDLER_NOEC(handler2, T_NMI);
	TRAPHANDLER_NOEC(handler3, T_BRKPT);
	TRAPHANDLER_NOEC(handler4, T_OFLOW);
	TRAPHANDLER_NOEC(handler5, T_BOUND);
	TRAPHANDLER_NOEC(handler6, T_ILLOP);
	TRAPHANDLER_NOEC(handler7, T_DEVICE);
	TRAPHANDLER_NOEC(handler8, T_DBLFLT);
//	TRAPHANDLER(handler9, T_COPROC); //
	TRAPHANDLER(handler10, T_TSS);
	TRAPHANDLER(handler11, T_SEGNP);
	TRAPHANDLER(handler12, T_STACK);
	TRAPHANDLER(handler13, T_GPFLT);
	TRAPHANDLER(handler14, T_PGFLT);
//	TRAPHANDLER(handler15, T_RES); // 
	TRAPHANDLER_NOEC(handler16, T_FPERR);
	TRAPHANDLER(handler17, T_ALIGN);
 	TRAPHANDLER(handler18, T_MCHK);
 	TRAPHANDLER(handler19, T_SIMDERR);
/*	TRAPHANDLER(handler20, T_DBLFLT);
 *	TRAPHANDLER(handler21, T_DBLFLT);
 *	TRAPHANDLER(handler22, T_DBLFLT);
 *	TRAPHANDLER(handler23, T_DBLFLT);
 *	TRAPHANDLER(handler24, T_DBLFLT);
 *	TRAPHANDLER(handler25, T_DBLFLT);
 *	TRAPHANDLER(handler26, T_DBLFLT);
 *	TRAPHANDLER(handler27, T_DBLFLT);
 *	TRAPHANDLER(handler28, T_DBLFLT);
 *	TRAPHANDLER(handler29, T_DBLFLT);
 *	TRAPHANDLER(handler30, T_DBLFLT);
 *	TRAPHANDLER(handler31, T_DBLFLT);
 *	TRAPHANDLER(handler32, T_DBLFLT);
 */
 	TRAPHANDLER_NOEC(handler48, T_SYSCALL);

/*
 * Lab 3: Your code here for _alltraps
 */
 _alltraps:
# push other registers to make a trapframe in kernel stack
	pushl	%ds
	pushl	%es
	pushal				# push all general-purpose registers
	
# set segments for trap handler
	mov		$GD_KD, %eax
	mov		%ax, %ds
	mov		%ax, %es

# jump to trap
	pushl	%esp		# pass a Trapframe pointer as an argument to trap()
	call	trap		# can trap ever return?

