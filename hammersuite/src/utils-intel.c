#include "utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#ifdef NUC
#include <sys/mman.h>
#include <sched.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "memory.h"

static int pmap_fd = NOT_OPENED;

/**
Inputs: None

Makes the calls to cache maintenance/random seeding/etc. that will be used during the runtime.

Output: None
*/
void startup() {
	srand(time(NULL));
}

/**
Inputs: mem - holds the flags & parameters for setting up memory

Helper to alloc_buffer, with Intel-specific features. Tries to align the buffer to a specified offset if mem->align isn't empty. Uses 1G or 2MB hugepages.

Output: alloc_size - the number of bytes that were allocated
*/
uint64_t build_buffer(MemoryBuffer* mem) {
    if (mem->align < _SC_PAGE_SIZE) {
            mem->align = 0;
    }

    uint64_t alloc_size = mem->align ? mem->size + mem->align : mem->size;
    uint64_t alloc_flags = MAP_PRIVATE | MAP_POPULATE;

    if (mem->flags & F_ALLOC_HUGE) {
        // fprintf(stderr," huge page\n");
        // if(mem->flags & F_ALLOC_HUGE_1G) fprintf(stderr, "1GB hugepage\n");
        if (mem->fd == 0) {
            fprintf(stderr,
                "[ERROR] - Missing file descriptor to allocate hugepage\n");
            exit(1);
        }
        alloc_flags |=
            (mem->flags & F_ALLOC_HUGE_1G) ? MAP_ANONYMOUS | MAP_HUGETLB
            | (30 << MAP_HUGE_SHIFT)
            : (mem->flags & F_ALLOC_HUGE_2M) ? MAP_ANONYMOUS |
            MAP_HUGETLB | (21 << MAP_HUGE_SHIFT)
            : MAP_ANONYMOUS;
    } else {
        mem->fd = -1;
        alloc_flags |= MAP_ANONYMOUS;
    }
    mem->buffer = (char *)mmap(NULL, mem->size, PROT_READ | PROT_WRITE,
                alloc_flags, mem->fd, 0);
    if (mem->buffer == MAP_FAILED) {
        perror("[ERROR] - mmap() failed");
        exit(1);
    }
    if (mem->align) {
        size_t error = (uint64_t) mem->buffer % mem->align;
        size_t left = error ? mem->align - error : 0;
        munmap(mem->buffer, left);
        mem->buffer += left;
        assert((uint64_t) mem->buffer % mem->align == 0);
    }
    return alloc_size;
}

/**
Inputs: mem - holds the pointer to the buffer

Frees and unmaps the memory allocated.

Output: 0 on success, -1 otherwise
*/
int tear_down_buff(MemoryBuffer* mem) {
    free(mem->physmap);
	return munmap(mem->buffer, mem->size);
}

/**
Inputs: p_addr - the physical address to get the column of
        g_mem_layout - the listing of DRAM functions

Using the mask in g_mem_layout, return the corresponding DRAM column of p_addr.

Output: the column p_addr maps to
*/
uint64_t get_dram_col(physaddr_t p_addr, DRAMLayout g_mem_layout)
{
	return (p_addr & g_mem_layout.
		col_mask) >> __builtin_ctzl(g_mem_layout.col_mask);
}

/**
Inputs: d_addr - the DRAM representation to extract the physical bits from

Given a DRAM representation d_addr, get the appropriate representation of column bits
   in the physical address. To be XORed with the row/bank bits to get a full physical address.

Output: the representation of this column as a physical address
*/
physaddr_t col_2_phys(DRAMAddr d_addr, DRAMLayout g_mem_layout) {
	return (d_addr.col << __builtin_ctzl(g_mem_layout.col_mask));	// set col bits
}

/**
Inputs: g_mem_layout

Write the DRAMLayout to memory, to be used in future runs.

Output: none
*/
void gmem_dump_helper(DRAMLayout g_mem_layout) {
	FILE *fp = fopen("g_mem_dump.bin", "wb+");
	fwrite(&g_mem_layout, sizeof(DRAMLayout), 1, fp);
	fclose(fp);

	#ifdef DEBUG
		DRAMLayout tmp;
		fp = fopen("g_mem_dump.bin", "rb");
		fread(&tmp, sizeof(DRAMLayout), 1, fp);
		fclose(fp);

		assert(tmp->h_fns->len == g_mem_layout->h_fns->len);
		assert(tmp->bank == g_mem_layout->bank);
		assert(tmp->row == g_mem_layout->row);
		assert(tmp->col == g_mem_layout->col);

	#endif
}

/**
Inputs: entry - the pagemap entry

Given a pagemap entry, return the page frame number.

Output: page frame number of the entry
*/
uint64_t get_pfn(uint64_t entry)
{
	return ((entry) & 0x7fffffffffffffff);
}

/**
Inputs: v_addr - the virtual address of the memory location, represented as a uint
        pmap_fd - fd for the pagemap table

Given a virtual address v_addr, find it in the pagemap and get its physical address.

Output: physical address corresponding to the virtual address
*/
physaddr_t get_physaddr(uint64_t v_addr, int pmap_fd)
{
	uint64_t entry;
	uint64_t offset = (v_addr / 4096) * sizeof(entry);
	uint64_t pfn;
	bool to_open = false;
	// assert(fd >= 0);
	if (pmap_fd == NOT_OPENED) {
		pmap_fd = open("/proc/self/pagemap", O_RDONLY);
		assert(pmap_fd >= 0);
		to_open = true;
	}
	// int rd = fread(&entry, sizeof(entry), 1 ,fp);
	int bytes_read = pread(pmap_fd, &entry, sizeof(entry), offset);

	assert(bytes_read == 8);
	assert(entry & (1ULL << 63));

	if (to_open) {
		close(pmap_fd);
	}

	pfn = get_pfn(entry);
	assert(pfn != 0);
	return (pfn << 12) | (v_addr & 4095);
}

/**
Inputs: p1, p2 - pointers to the pseudo-page table entries

Comparison function, used in the call to qsort. 

Output: whichever pte has a larger physical address
*/
int phys_cmp(const void *p1, const void *p2)
{
	return ((pte_t *) p1)->p_addr - ((pte_t *) p2)->p_addr;
}

/**
Inputs: mem - holds the buffer data for reading the physmap

Sets up the physmap--a sorted pagemap for the buffer

Output: none
*/
// WARNING optimization works only with contiguous memory!!
void set_physmap(MemoryBuffer * mem)
{
	int l_size = mem->size / PAGE_SIZE;
	pte_t *physmap = (pte_t *) malloc(sizeof(pte_t) * l_size);
	int pmap_fd = open("/proc/self/pagemap", O_RDONLY);
	assert(pmap_fd >= 0);

	base_phys = get_physaddr((uint64_t) mem->buffer, pmap_fd);
	for (uint64_t tmp = (uint64_t) mem->buffer, idx = 0;
	     tmp < (uint64_t) mem->buffer + mem->size; tmp += PAGE_SIZE) {
		pte_t tmp_pte = { (char *)tmp, get_physaddr(tmp, pmap_fd) };
		physmap[idx] = tmp_pte;
		idx++;
	}

	qsort(physmap, mem->size / PAGE_SIZE, sizeof(pte_t), phys_cmp);
	close(pmap_fd);
	mem->physmap = physmap;
}

/**
Inputs: p_addr - the physical address to convert to virtual addr
        mem - holds the physmap for searching virt. addrs

Given a physical address, searches the physmap for the corresponding pseudo PTE.
Converts the PTE to the virtual address referencing that physical address.

Output: the virtual address of the given physical address
*/
char* phys_2_virt_helper(physaddr_t p_addr, MemoryBuffer* mem) {
	physaddr_t p_page = p_addr & ~(((uint64_t) PAGE_SIZE - 1));
	pte_t src_pte = {.v_addr = 0,.p_addr = p_page };
	pte_t *res_pte =
	    (pte_t *) bsearch(&src_pte, mem->physmap, mem->size / PAGE_SIZE,
			      sizeof(pte_t), phys_cmp);

	if (res_pte == NULL)
		return (char *)NOT_FOUND;

    uint64_t physical_base = get_physaddr((uint64_t)mem->buffer, pmap_fd);

	// fprintf(stderr, "phy_base: %lx\n", physical_base);

	return (char *)((uint64_t) res_pte->
			v_addr | ((uint64_t) p_addr &
				  (((uint64_t) PAGE_SIZE - 1))));
	// assert(false);

    // return (char *)((physical_base & ~(((uint64_t) PAGE_SIZE - 1))) | ((uint64_t) p_addr & (((uint64_t) PAGE_SIZE - 1))));
}

/**
Inputs: none

Yields the CPU, if such a call exists. Helps prevent us from getting "kicked off" the CPU once we start hammering.

Output: none
*/
void sched_yield_helper() {
	sched_yield();
}

/**
Inputs: p - holds the profile parameters

If we can't pass arguments to the program, use this function to fill them in with hardcodes.

Output: none
*/
void manually_fill_params(ProfileParams* p) {
	return;
}

/**
Inputs: dir_name - the name of the directory to create

Creates a directory to house the files we generate, if it doesn't exist.

Output: none
*/
void create_dir(const char* dir_name)
{
	struct stat st = {0};
	if (stat(dir_name, &st) == -1) {
			mkdir(dir_name, 0777);
	}
	return;
}

/**
Inputs: CL_SEED - the randomness seed

Read the CL_SEED from /dev/urandom, if we don't have one already.

Output: none
*/
void read_random(uint64_t CL_SEED) {
	int fd;
	if ((fd = open("/dev/urandom", O_RDONLY)) == -1) {
		perror("[ERROR] - Unable to open /dev/urandom");
		exit(1);
	}
	if (CL_SEED == 0) {
		if (read(fd, &CL_SEED, sizeof(CL_SEED)) == -1) {
			perror("[ERROR] - Unable to read /dev/urandom");
			exit(1);
		}
	}
	fprintf(stderr,"#seed: %lx\n", CL_SEED);
	close(fd);
}

/**
Inputs: p - holds the fd of the hugetlb

Ensures the hugetlbfs can be opened.

Output: 0 if successful, -1 otherwise
*/
int open_hugetlb(ProfileParams* p) {
	if ((p->huge_fd = open(p->huge_file, O_CREAT | O_RDWR, 0755)) == -1) {
		perror("[ERROR] - Unable to open hugetlbfs");
		return -1;
	}
	return 0;
}