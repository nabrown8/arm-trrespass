#ifdef ZUBOARD
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

#define PAGE_BITS 12

#define FLAGS (MAP_PRIVATE | MAP_POPULATE | MAP_HUGETLB | (30<<MAP_HUGE_SHIFT))

void startup() {
	srand(12345678);
    enable_pmccntr();
    disable_caches();
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
    mem->buffer = (char *)malloc(mem->size);
    if (mem->buffer == NULL) {
        perror("[ERROR] - malloc failed");
        exit(1);
    }
    return mem->size;
}

int tear_down_buff(MemoryBuffer* mem) {
    free(mem->buffer);
    return 0;
}

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

physaddr_t col_2_phys(DRAMAddr d_addr, DRAMLayout g_mem_layout) {
	physaddr_t phys = 0;
	int colbit = 0;
	for (int i = 0; i < 31; i++) {
		if ((g_mem_layout.col_mask >> i) % 2 == 0) continue;
		colbit = d_addr % 2;
		if (colbit) {
			phys |= (1 << i);
		}
		d_addr >>= 1;
	}
	return phys;
}

void gmem_dump_helper(DRAMLayout g_mem_layout) {
	return; // can't use this
}

uint64_t get_pfn(uint64_t entry)
{
	return 0;
}

physaddr_t get_physaddr(uint64_t v_addr, int pmap_fd)
{
	return (physaddr_t) v_addr;
}

int phys_cmp(const void *p1, const void *p2)
{
	return 0;
}

// WARNING optimization works only with contiguous memory!!
void set_physmap(MemoryBuffer * mem)
{
	return;
}

char* phys_2_virt_helper(physaddr_t p_addr, MemoryBuffer* mem) {
    return (char*) p_addr;
}
#endif