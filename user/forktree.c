// Fork a binary tree of processes and display their structure.

#include <inc/lib.h>

#define DEPTH 3

void forktree(const char *cur);

void
forkchild(const char *cur, char branch)
{
cprintf("begin forkchild\n\n");
	char nxt[DEPTH+1];

	if (strlen(cur) >= DEPTH)
		return;

	snprintf(nxt, DEPTH+1, "%s%c", cur, branch);
	if (fork() == 0) {
cprintf("after fork\n\n");
		forktree(nxt);
		exit();
	}
}

void
forktree(const char *cur)
{
	cprintf("%04x: I am '%s'\n", sys_getenvid(), cur);

	forkchild(cur, '0');
cprintf("after forkchild 0\n\n");
	forkchild(cur, '1');
}

void
umain(int argc, char **argv)
{
	forktree("");
}

