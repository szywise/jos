// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

int mon_showmappings(int argc, char ** argv, struct Trapframe *tf);
int mon_chperm(int argc, char ** argv, struct Trapframe * tf);

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display backtrace debug infomation", mon_backtrace },
	{ "showmappings", "Display physical page mappings", mon_showmappings },
	{ "chperm", "Change the permission of a virtual page", mon_chperm },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Stack backtrace:\n");
	uint32_t* ebp_mon = (uint32_t*) read_ebp(); // typedef uintptr_t
	uint32_t eip_mon = 0;
	uint32_t arg[5] = {0};
	do
	{
		eip_mon = *(ebp_mon + 1);
		arg[0] = *(ebp_mon + 2);
		arg[1] = *(ebp_mon + 3);
		arg[2] = *(ebp_mon + 4);
		arg[3] = *(ebp_mon + 5);
		arg[4] = *(ebp_mon + 6);
		cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n",
			ebp_mon, eip_mon, arg[0], arg[1], arg[2], arg[3], arg[4]);

		struct Eipdebuginfo info;
		debuginfo_eip(eip_mon, &info);
		cprintf("         %s:%d: %.*s+%d\n",
			info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, 
			eip_mon - info.eip_fn_addr);

		ebp_mon = (uint32_t *) *ebp_mon;
	}
	while(ebp_mon != 0);
	return 0;
}

int
mon_showmappings(int argc, char ** argv, struct Trapframe *tf)
{
	if(argc != 3) {
		cprintf("usage: showmappings VA_BEGIN VA_END\n");
		return 0;
	}
	// \t隔开的两个至少空6格
	// printf: %[flags][width][.perc][F|N|h|l]type
	cprintf("  VADDR       PADDR       PTE_U   PTE_W   PTE_P\n");
	char *end_char;
	// 对于uintptr_t，加１时它的值加１还是加４??
	uintptr_t va_beg = ROUNDDOWN(strtol(argv[1], &end_char, 16), PGSIZE);
	if(*end_char != '\0') goto mon_showmappings_arg_error;
	uintptr_t va_end = ROUNDDOWN(strtol(argv[2], &end_char, 16), PGSIZE);
	if(*end_char != '\0') goto mon_showmappings_arg_error;
	for(; va_beg <= va_end; va_beg += PGSIZE) {
		pte_t * pte_ptr = pgdir_walk(kern_pgdir, (void *)va_beg, 0);
		if(pte_ptr == NULL) // no page table
			cprintf("  0x%08x  -           -       -       -\n", va_beg);
		else if(!(*pte_ptr & PTE_P)) // page not present
			cprintf("  0x%08x  -           -       -       0\n", va_beg);
		else
			cprintf("  0x%08x  0x%08x  %d       %d       %d\n",
				va_beg, PTE_ADDR(*pte_ptr),
				!!(*pte_ptr & PTE_U), !!(*pte_ptr & PTE_W), !!(*pte_ptr & PTE_P));
	}
	return 0;
mon_showmappings_arg_error:
	cprintf("showmappings: ERROR: parameters not correct. See -h\n");
	return 0; // 1?
}

int
mon_chperm(int argc, char ** argv, struct Trapframe * tf)
{
	if(argc != 3) {
		cprintf("Usage: chperm VADDR PERM\n");
		cprintf("Change the permission of vitual address VADDR to PERM.\n");
		cprintf(" PERM:\t0\t2\t4\t6\n");
		cprintf(" KERN:\tR\tRW\tRW\tRW\n"); // 4 - KERN RW ??
		cprintf(" USER:\t-\t-\tR\tRW\n");
		return 0;
	}
	char* end_char;
	uintptr_t vaddr = ROUNDDOWN(strtol(argv[1], &end_char, 16), PGSIZE);
	if(*end_char != '\0') goto mon_chperm_arg_error;
	uintptr_t perm = strtol(argv[2], &end_char, 10);
	if(*end_char != '\0') goto mon_chperm_arg_error;
	pte_t * pte_ptr = pgdir_walk(kern_pgdir, (void *)vaddr, 0);
	if(pte_ptr == NULL || !(*pte_ptr & PTE_P)) {
		cprintf("ERROR: page not present!\n");
		return 0;
	}
	perm ++; // present-bit = 1;
	*pte_ptr = (*pte_ptr >> 3 << 3) | perm;
	return 0;

mon_chperm_arg_error:
	cprintf("ERROR: parameters not correct!\n");
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	cprintf("x=%d y=%d\n", 3);
	cprintf("\033[1;45;33m HELLO WORLD \033[0m\n");

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
