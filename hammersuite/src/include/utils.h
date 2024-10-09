#pragma once

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "dram-address.h"
#include "params.h"

#define NUC 0
// #define ZUBOARD 1

#define BIT_SET(x) 		(1ULL<<(x))
#define BIT_VAL(b,val) 	(((val) >> (b)) & 1)
#define KB(x) 			((x)<<10ULL)
#define MB(x) 			((x)<<20ULL)
#define GB(x) 			((x)<<30ULL)
#define CL_SHIFT 		6
#define CL_SIZE 		(1<<6)
#define PAGE_SIZE 		(1<<30)
#define ROW_SIZE 		(8<<10)

#define ALIGN_TO(X, Y) ((X) & (~((1LL<<(Y))-1LL)))	// Mask out the lower Y bits
#define LS_BITMASK(X)  ((1LL<<(X))-1LL)	// Mask only the lower X bits

// Flags 
#define F_CLEAR 			0L
#define F_VERBOSE 			BIT_SET(0)
#define F_EXPORT 			BIT_SET(1)
#define F_CONFIG			BIT_SET(2)
#define F_NO_OVERWRITE		BIT_SET(3)
#define MEM_SHIFT			(30L)
#define MEM_MASK			0b11111ULL << MEM_SHIFT
#define F_ALLOC_HUGE 		BIT_SET(MEM_SHIFT)
#define F_ALLOC_HUGE_1G 	F_ALLOC_HUGE | BIT_SET(MEM_SHIFT+1)
#define F_ALLOC_HUGE_2M		F_ALLOC_HUGE | BIT_SET(MEM_SHIFT+2)
#define F_POPULATE			BIT_SET(MEM_SHIFT+3)

#define NOT_FOUND 	((void*) -1)
#define	NOT_OPENED  -1
#define NO_FS ((FILE*) NULL)

#define TIMESPEC_NSEC(ts) ((ts)->tv_sec * 1e9 + (ts)->tv_nsec)

static physaddr_t base_phys = 0L;

// void set_physmap(mem_buff_t* mem);

// pte_t get_pte(char* v_addr, mem_buff_t* mem);

// addr_tuple reverse_addr_tuple(uint64_t p_addr, mem_buff_t* mem);

//----------------------------------------------------------
//                      Helpers 
void startup();

int gt(const void *a, const void *b);

double mean(uint64_t * vals, size_t size);

uint64_t median(uint64_t * vals, size_t size);

char *bit_string(uint64_t val);

char *int_2_bin(uint64_t val);

char *get_rnd_addr(char *base, size_t m_size, size_t align);

int get_rnd_int(int min, int max);

uint64_t build_buffer(MemoryBuffer* mem);

int tear_down_buff(MemoryBuffer* mem);

uint64_t get_dram_col(physaddr_t p_addr, DRAMLayout g_mem_layout);

physaddr_t col_2_phys(DRAMAddr d_addr, DRAMLayout g_mem_layout);

void set_physmap_helper(MemoryBuffer* mem);

void gmem_dump_helper(DRAMLayout g_mem_layout);

uint64_t get_pfn(uint64_t entry);

physaddr_t get_physaddr(uint64_t v_addr, int pmap_fd);

int phys_cmp(const void *p1, const void *p2);

// WARNING optimization works only with contiguous memory!!
void set_physmap(MemoryBuffer * mem);

char* phys_2_virt_helper(physaddr_t p_addr, MemoryBuffer* mem);

void sched_yield_helper();

void manually_fill_params(ProfileParams* p);

void create_dir(const char* dir_name);

void read_random(uint64_t CL_SEED);

int open_hugetlb(ProfileParams* p);