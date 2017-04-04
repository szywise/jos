// program to cause a breakpoint trap

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $3");
/*	asm volatile("push %%ebx\n\t"
				"mov %%eax, %%ebx\n\t"
				"mov $0x1234, %%eax\n\t"
				"int $3\n\t"
				"movl %%ebx, %%eax\n\t"
				"int $3\n\t"
				"popl %%ebx"
				:
				:
				:);
	cprintf("[HELLO]@!@##$%%!\n\n");
*/
}

