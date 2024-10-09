#pragma once

#include <stdint>

#include "dram-address.h"
#include "params.h"

/**
Inputs: p - the virtual address to be flushed

Ensures the value stored by p is not contained in any of the system's caches--in other words, it will next be served from memory.

Output: None
*/
static inline __attribute__ ((always_inline))
void clflush(volatile void *p)
{
	asm volatile ("clflush (%0)\n"::"r" (p):"memory");
}

/**
Inputs: p - the virtual address to be flushed

An optimized version of clflush. Has the same guarantees, but supposedly increases throughput.

Output: None
*/
static inline __attribute__ ((always_inline))
void clflushopt(volatile void *p)
{
#ifdef DDR3
	asm volatile ("clflush (%0)\n"::"r" (p):"memory");
#else
	asm volatile ("clflushopt (%0)\n"::"r" (p):"memory");
#endif
}

/**
Inputs: none

Used as a makeshift serializer?

Output: None
*/
static inline __attribute__ ((always_inline))
void cpuid()
{
	asm volatile ("cpuid":::"rax", "rbx", "rcx", "rdx");
}

/**
Inputs: none

Serializes all loads and stores. In other words, all the loads/stores before the mfence are globally visible before any loads/stores after it.

Output: None
*/
static inline __attribute__ ((always_inline))
void mfence()
{
	asm volatile ("mfence":::"memory");
}

/**
Inputs: none

Like mfence, but only for stores. Loads are unaffected.

Output: None
*/
static inline __attribute__ ((always_inline))
void sfence()
{
	asm volatile ("sfence":::"memory");
}

/**
Inputs: none

Like mfence, but only for loads. Stores are unaffected.

Output: None
*/
static inline __attribute__ ((always_inline))
void lfence()
{
	asm volatile ("lfence":::"memory");
}

/**
Inputs: none

Returns the timestamp counter to the user, and ensures the instruction pipeline is cleared.

Output: timestamp counter value
*/
static inline __attribute__ ((always_inline))
uint64_t rdtscp(void)
{
	uint64_t lo, hi;
	asm volatile ("rdtscp\n":"=a" (lo), "=d"(hi)
		      ::"%rcx");
	return (hi << 32) | lo;
}

/**
Inputs: none

Returns the timestamp counter to the user. Doesn't clear the pipeline.

Output: timestamp counter value
*/
static inline __attribute__ ((always_inline))
uint64_t rdtsc(void)
{
	uint64_t lo, hi;
	asm volatile ("rdtsc\n":"=a" (lo), "=d"(hi)
		      ::"%rcx");
	return (hi << 32) | lo;
}

/**
Inputs: none

Get the current clock reading in ns.

Output: clock reading in ns
*/
static inline __attribute__ ((always_inline))
uint64_t realtime_now()
{
	struct timespec now_ts;
	clock_gettime(CLOCK_MONOTONIC, &now_ts);
	return TIMESPEC_NSEC(&now_ts);
}

/**
Inputs: none

Using the CL_SEED, generates pseudo-random values via the CRC builtin.

Output: the buffer of randomized values
*/
static inline __attribute((always_inline))
char *cl_rand_gen(DRAMAddr * d_addr, uint64_t CL_SEED)
{
	static uint64_t cl_buff[8];
	for (int i = 0; i < 8; i++) {
		cl_buff[i] =
			__builtin_ia32_crc32di(CL_SEED,
				(d_addr->row + d_addr->bank +
				(d_addr->col + i*8)));
	}
	return (char *)cl_buff;
}

uint64_t build_buffer(MemoryBuffer* mem);

int tear_down_buff(MemoryBuffer* mem);

uint64_t get_dram_col(physaddr_t p_addr, DRAMLayout g_mem_layout);

physaddr_t col_2_phys(DRAMAddr d_addr, DRAMLayout g_mem_layout);

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