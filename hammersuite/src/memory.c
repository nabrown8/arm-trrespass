#include "memory.h"
#include "utils.h"

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef NUC
#include <sys/mman.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#ifdef NUC
#include "utils-intel.h"
#elif defined ZUBOARD
#include "utils-arm.h"
#endif

#define DEF_RNG_LEN (8<<10)
#define DEBUG
#define DEBUG_LINE fprintf(stderr, "[DEBUG] - GOT HERE\n");

static int pmap_fd = NOT_OPENED;

physaddr_t virt_2_phys(char *v_addr, MemoryBuffer * mem)
{
	// for (int i = 0; i < mem->size / PAGE_SIZE; i++) {
	// 	if (mem->physmap[i].v_addr ==
	// 	    (char *)((uint64_t) v_addr & ~((uint64_t) (PAGE_SIZE - 1))))
	// 	{

	// 		return mem->physmap[i].
	// 		    p_addr | ((uint64_t) v_addr &
	// 			      ((uint64_t) PAGE_SIZE - 1));
	// 	}
	// }
	// return (physaddr_t) NOT_FOUND;
	
	return get_physaddr((uint64_t)v_addr, pmap_fd);

	
}


char *phys_2_virt(physaddr_t p_addr, MemoryBuffer * mem)
{
	return phys_2_virt_helper(p_addr, mem);
}
