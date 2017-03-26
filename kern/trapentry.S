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
devide:
TRAPHANDLER("divide", T_DIVIDE);

TRAPHANDLER("debug", T_DEBUG);
TRAPHANDLER("divide", T_NMI);
TRAPHANDLER("divide", T_BRKPT);
TRAPHANDLER("divide", T_OFLOW);
TRAPHANDLER("divide", T_BOUND);
TRAPHANDLER("divide", T_ILLOP);
TRAPHANDLER("divide", T_DEVICE);


/*
 * Lab 3: Your code here for _alltraps
 */
 _alltraps:
	pushal				#push values, to make the stack look like a struct Trapframe
	mov		GD_KD, %ds
	mov		GD_KD, %es
	pushl	%esp		# pass a pointer to the Trapframe, as an argument to trap()
	call	trap		# can trap ever return?

