#ifdef ZUBOARD
#include "utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#define PAGE_BITS 12

#define FLAGS (MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB | (30<<MAP_HUGE_SHIFT))
\/**
Inputs: None

Makes the calls to cache maintenance/random seeding/etc. that will be used during the runtime.

DIFF: No time support, so we seed a fixed value. Have to turn on the cycle counter and disable the data caches.

Output: None
*/
void startup() {
	srand(12345678);
    enable_pmccntr();
    disable_caches();
}

/**
Inputs: mem - holds the flags & parameters for setting up memory

Helper to alloc_buffer, with Intel-specific features. Tries to align the buffer to a specified offset if mem->align isn't empty. Uses 1G or 2MB hugepages.

DIFF: Without Linux, can't use mmap -- we use malloc instead.

Output: alloc_size - the number of bytes that were allocated
*/
uint64_t build_buffer(MemoryBuffer* mem) {
    mem->buffer = (char *)malloc(mem->size);
    if (mem->buffer == NULL) {
        perror("[ERROR] - malloc failed");
        exit(1);
    }
    return mem->size;
}

/**
Inputs: mem - holds the pointer to the buffer

Frees and unmaps the memory allocated.

Output: 0 on success, -1 otherwise
*/
int tear_down_buff(MemoryBuffer* mem) {
    free(mem->buffer);
    return 0;
}

/**
Inputs: p_addr - the physical address to get the column of
        g_mem_layout - the listing of DRAM functions

Using the mask in g_mem_layout, return the corresponding DRAM column of p_addr.

DIFF: Since column bits aren't consecutive, we can't just downshift. Instead, we look at each bit of the phys. addr,
		compare it to the mask, and change the col value if that bit is set in both the physical address & mask.

Output: the column p_addr maps to
*/
uint64_t get_dram_col(physaddr_t p_addr, DRAMLayout g_mem_layout)
{
    uint64_t col = 0;
    uint64_t colbit = 0;
    for (uint64_t i = 0; i < 31; i++) {
        if ((g_mem_layout.col_mask >> i) % 2 == 0) continue;
        col += (((p_addr >> i) % 2) << colbit);
        colbit++;
    }
    return col;
}

/**
Inputs: d_addr - the DRAM representation to extract the physical bits from

Given a DRAM representation d_addr, get the appropriate representation of column bits
   in the physical address. To be XORed with the row/bank bits to get a full physical address.

DIFF: Same as above.

Output: the representation of this column as a physical address
*/
physaddr_t col_2_phys(DRAMAddr d_addr, DRAMLayout g_mem_layout) {
	physaddr_t phys = 0;
	uint64_t col = d_addr.col;
	int colbit = 0;
	for (int i = 0; i < 31; i++) {
		if ((g_mem_layout.col_mask >> i) % 2 == 0) continue;
		colbit = col % 2;
		if (colbit) {
			phys |= (1 << i);
		}
		col >>= 1;
	}
	return phys;
}

/**
Inputs: g_mem_layout

Write the DRAMLayout to memory, to be used in future runs. DUMMY.

Output: none
*/
void gmem_dump_helper(DRAMLayout g_mem_layout) {
	return; // can't use this
}

/**
Inputs: entry - the pagemap entry

Given a pagemap entry, return the page frame number. DUMMY.

Output: page frame number of the entry
*/
uint64_t get_pfn(uint64_t entry)
{
	return 0;
}

/**
Inputs: v_addr - the virtual address of the memory location, represented as a uint
        pmap_fd - fd for the pagemap table

Given a virtual address v_addr, find it in the pagemap and get its physical address.

DIFF: With the MMU disabled, virt. addrs are effectively the same as physical addresses.

Output: physical address corresponding to the virtual address
*/
physaddr_t get_physaddr(uint64_t v_addr, int pmap_fd)
{
	return (physaddr_t) v_addr;
}

/**
Inputs: p1, p2 - pointers to the pseudo-page table entries

Comparison function, used in the call to qsort. DUMMY.

Output: whichever pte has a larger physical address
*/
int phys_cmp(const void *p1, const void *p2)
{
	return 0;
}

/**
Inputs: mem - holds the buffer data for reading the physmap

Sets up the physmap--a sorted pagemap for the buffer. DUMMY.

Output: none
*/
// WARNING optimization works only with contiguous memory!!
void set_physmap(MemoryBuffer * mem)
{
	return;
}

/**
Inputs: p_addr - the physical address to convert to virtual addr
        mem - holds the physmap for searching virt. addrs

Given a physical address, searches the physmap for the corresponding pseudo PTE.
Converts the PTE to the virtual address referencing that physical address.

DIFF: Same as get_physaddr. Can just cast.

Output: the virtual address of the given physical address
*/
char* phys_2_virt_helper(physaddr_t p_addr, MemoryBuffer* mem) {
    return (char*) p_addr;
}

/**
Inputs: none

Yields the CPU, if such a call exists. Helps prevent us from getting "kicked off" the CPU once we start hammering. DUMMY.

Output: none
*/
void sched_yield_helper() {
	return;
}

/**
Inputs: p - holds the profile parameters

If we can't pass arguments to the program, use this function to fill them in with hardcodes.

Output: none
*/
void manually_fill_params(ProfileParams* p) {
	p->m_size = 1000000000;
	p->m_align = 0;
	p->g_flags = F_VERBOSE;
	p->base_off = 250;
	p->aggr = 9;
	p->rounds = 1000000;
	p->fuzzing = 0;
	p->vpat = NULL;
	p->tpat = NULL;
}

/**
Inputs: dir_name - the name of the directory to create

Creates a directory to house the files we generate, if it doesn't exist.

Output: none
*/
void create_dir(const char* dir_name)
{
	return;
}

/**
Inputs: CL_SEED - the randomness seed

Read the CL_SEED from /dev/urandom, if we don't have one already.

Output: none
*/
void read_random(uint64_t CL_SEED) {
	return;
}

/**
Inputs: p - holds the fd of the hugetlb

Ensures the hugetlbfs can be opened.

Output: 0 if successful, -1 otherwise
*/
int open_hugetlb(ProfileParams* p) {
	return 0;
}
#endif