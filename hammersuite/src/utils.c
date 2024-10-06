#include "utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "memory.h"

#define PAGE_BITS 12

#define FLAGS (MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB | (30<<MAP_HUGE_SHIFT))

static int pmap_fd = NOT_OPENED;

void startup() {
	srand(time(NULL));
}

char *get_rnd_addr(char *base, size_t m_size, size_t align)
{
	return (char *)((((uint64_t) base) + (rand() % m_size)) &
			(~((uint64_t) align - 1)));
}

int get_rnd_int(int min, int max)
{
	return rand() % (max + 1 - min) + min;
}

double mean(uint64_t * vals, size_t size)
{
	uint64_t avg = 0;
	for (size_t i = 0; i < size; i++) {
		avg += vals[i];
	}
	return ((double)avg) / size;
}

int gt(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}

uint64_t median(uint64_t * vals, size_t size)
{
	qsort(vals, size, sizeof(uint64_t), gt);
	return ((size % 2) ==
		0) ? vals[size / 2] : (vals[(size_t) size / 2] +
				       vals[((size_t) size / 2 + 1)]) / 2;
}

char *bit_string(uint64_t val)
{
	static char bit_str[256];
	char itoa_str[8];
	strcpy(bit_str, "");
	for (int shift = 0; shift < 64; shift++) {
		if ((val >> shift) & 1) {
			if (strcmp(bit_str, "") != 0) {
				strcat(bit_str, "+ ");
			}
			sprintf(itoa_str, "%d ", shift);
			strcat(bit_str, itoa_str);
		}
	}

	return bit_str;
}

char *int_2_bin(uint64_t val)
{
	static char bit_str[256];
	char itoa_str[8];
	strcpy(bit_str, "0b");
	for (int shift = 64 - __builtin_clzl(val); shift >= 0; --shift) {
		sprintf(itoa_str, "%d", (int)(val >> shift) & 1);
		strcat(bit_str, itoa_str);
	}

	return bit_str;
}

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

int tear_down_buff(MemoryBuffer* mem) {
    free(mem->physmap);
	return munmap(mem->buffer, mem->size);
}

uint64_t get_dram_col(physaddr_t p_addr, DRAMLayout g_mem_layout)
{
	return (p_addr & g_mem_layout.
		col_mask) >> __builtin_ctzl(g_mem_layout.col_mask);
}

physaddr_t col_2_phys(DRAMAddr d_addr, DRAMLayout g_mem_layout) {
	return (d_addr.col << __builtin_ctzl(g_mem_layout.col_mask));	// set col bits
}

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

uint64_t get_pfn(uint64_t entry)
{
	return ((entry) & 0x7fffffffffffffff);
}

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

int phys_cmp(const void *p1, const void *p2)
{
	return ((pte_t *) p1)->p_addr - ((pte_t *) p2)->p_addr;
}

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
