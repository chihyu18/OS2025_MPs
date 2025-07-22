#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"

/* Page fault handler */
int handle_pgfault()
{
    /* Find the address that caused the fault */
    /* uint64 va = r_stval(); */

    /* mp3 TODO */
    /* Find the address that caused the fault */
    uint64 va = r_stval();
    struct proc *p = myproc();

    /* Check if the address is within the user space */
    if (va >= p->sz || va < PGROUNDDOWN(p->trapframe->sp)) {
        printf("Page fault at invalid address: %p\n", va);
        return -1; // Invalid address
    }
    /* check if the page is swapped, if so, bring it in? */
    pte_t *pte = walk(p->pagetable, va, 0);
    if (pte && (*pte & PTE_S)) {
        // swap in
        uint blkno = PTE2BLOCKNO(*pte);
        char *mem = kalloc();
        if (mem == 0) {
            return -1; // out of memory
        }
        begin_op();
        read_page_from_disk(ROOTDEV, mem, blkno);
        bfree_page(ROOTDEV, blkno); // free the block number
        end_op();
        if (mappages(p->pagetable, PGROUNDDOWN(va), PGSIZE, (uint64)mem, PTE_R | PTE_W | PTE_X | PTE_U) < 0) {
            kfree(mem);
            return -1;
        }
        *pte = PA2PTE((uint64)mem) | (PTE_FLAGS(*pte) & ~PTE_S) | PTE_V;
        return 0;
    }
    /* Allocate a new page */
    char *mem = kalloc();
    if (mem == 0) {
        printf("Page fault: out of memory at address: %p\n", va);
        return -1; // Out of memory
    }
    /* Clear the allocated page */
    memset(mem, 0, PGSIZE);
    /* Map the new page into the process's page table */
    if (mappages(p->pagetable, PGROUNDDOWN(va), PGSIZE, (uint64)mem, PTE_R | PTE_W | PTE_U | PTE_X) < 0) {
        kfree(mem);
        printf("Page fault: failed to map page at address: %p\n", va);
        return -1; // Failed to map page
    }
    return 0; // Page fault handled successfully
    
}
