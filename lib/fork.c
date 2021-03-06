// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	pte_t pte = (pte_t)(uvpt[PGNUM(addr)]);
	if(!(err & FEC_WR) || !(pte & PTE_COW))
		panic("faulting access is not a write to a COW page");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	if((r = sys_page_alloc(0, (void*)PFTEMP, PTE_U | PTE_W | PTE_P)) < 0)
		panic("sys_page_alloc: %e", r);
	void* addr_pg = ROUNDDOWN(addr, PGSIZE);
	memcpy(PFTEMP, addr_pg, PGSIZE);
	if((r = sys_page_map(0, PFTEMP, 0, addr_pg, PTE_U | PTE_W | PTE_P)) < 0)
		panic("sys_page_map: %e", r);
	if((r = sys_page_unmap(0, PFTEMP)) < 0)
		panic("sys_page_unmap: %e", r);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	void* addr = (void*) (pn * PGSIZE);
	if(uvpt[PGNUM(addr)] & (PTE_W | PTE_COW)) {
		if((r = sys_page_map(0, addr, envid, addr, PTE_COW | PTE_U | PTE_P)) < 0)
			return r;
		if((r = sys_page_map(0, addr, 0, addr, PTE_COW | PTE_U | PTE_P)) < 0)
			return r;
	}
	else
		if((r = sys_page_map(0, addr, envid, addr, PTE_U | PTE_P)) < 0)
			// don't use 'PGOFF(uvpt[PGNUM(addr)])' as perm, because
			// the page from srcenv may be accessed or dirty, and we don't
			// want dstenv's newly-mapped page to be tagged as 'accessed'
			// or dirty. This is checked in sys_page_map
			return r;
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;
	int r;

	set_pgfault_handler(pgfault);

	envid = sys_exofork();
	if(envid < 0)
		panic("sys_exofork: %e", envid);
	if(envid == 0) {
		// We're the child.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// We're the parent.
	uintptr_t addr = 0;
	uintptr_t addr_top;
	int perm;
	while(addr < UTOP - PGSIZE){
		if(!(uvpd[PDX(addr)] & PTE_P)) {
			addr += PTSIZE;
			continue;
		}
		addr_top = addr + PTSIZE;
		if(addr_top == UTOP)
			addr_top -= PGSIZE; // skip User Exception Stack
		while(addr < addr_top) {
			perm = uvpt[PGNUM(addr)] & 7; // should I take lower 12 bits or 3 bits?
			if(!(perm & PTE_P)) {
				addr += PGSIZE;
				continue;
			}
			if((r = duppage(envid, PGNUM(addr))) < 0)
				panic("duppage failed at addr 0x%08x: %e", addr, r);
			addr += PGSIZE;
		}
	}

	if((r = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P)) < 0)
		panic("sys_page_alloc() failed: %e", r);
	extern void _pgfault_upcall(void);
	if((r = sys_env_set_pgfault_upcall(envid, (void*)_pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall() failed: %e", r);
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status failed: %e", r);

	return envid;
}

// Challenge!
int
sfork(void)
{
	envid_t envid;
	int r;

	set_pgfault_handler(pgfault);

	envid = sys_exofork();
	if(envid < 0)
		panic("sys_exofork: %e", envid);
	if(envid == 0) {
		// We're the child.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	// We're the parent.
	uintptr_t addr = 0;
	uintptr_t addr_top;
	int perm;
	while(addr < UTOP - 3*PGSIZE){
		if(!(uvpd[PDX(addr)] & PTE_P)) {
			addr += PTSIZE;
			continue;
		}
		addr_top = addr + PTSIZE;
		if(addr_top == UTOP)
			addr_top -= 3*PGSIZE; // skip User Exception Stack
		while(addr < addr_top) {
			perm = uvpt[PGNUM(addr)] & 7; // should I take lower 12 bits or 3 bits?
			// you should take lower 3 bits, because 'accessed' and 'dirty' are NOT 
			// permissions; they are status.
			if(!(perm & PTE_P)) {
				addr += PGSIZE;
				continue;
			}
			if((r = sys_page_map(0, (void *)addr, envid, (void *)addr, perm)) < 0)
				panic("sys_page_map failed at 0x%08x: %e", addr, r);
			addr += PGSIZE;
		}
	}

	if((r = duppage(envid, PGNUM(USTACKTOP-PGSIZE))) < 0)
		panic("duppage of normal user stack failed: %e");
	if((r = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U | PTE_W | PTE_P)) < 0)
		panic("sys_page_alloc() failed: %e", r);
	extern void _pgfault_upcall(void);
	if((r = sys_env_set_pgfault_upcall(envid, (void*)_pgfault_upcall)) < 0)
		panic("sys_env_set_pgfault_upcall() failed: %e", r);
	if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
		panic("sys_env_set_status: %e", r);

	return envid;
}
