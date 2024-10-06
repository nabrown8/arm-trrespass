#include "allocator.h"

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

#include "utils.h"

int alloc_buffer(MemoryBuffer * mem)
{
	if (mem->buffer != NULL) {
		fprintf(stderr, "[ERROR] - Memory already allocated\n");
	}

	uint64_t alloc_size = build_buffer(mem);

	// if (mem->flags & F_VERBOSE) {
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
		fprintf(stderr, "[ MEM ] - Buffer:      %p\n", mem->buffer);
		fprintf(stderr, "[ MEM ] - Size:        %ld\n", alloc_size);
		fprintf(stderr, "[ MEM ] - Alignment:   %ld\n", mem->align);
		fprintf(stderr, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	// }
	return 0;

}

int free_buffer(MemoryBuffer * mem)
{
	return tear_down_buff(mem);
}

