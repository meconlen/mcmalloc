#include <config.h>
#include "mcmalloc.h"

#include <inttypes.h>		// XintY_t
#include <stddef.h> 		// NULL
#include <stdio.h>			// fopen()
#include <stdlib.h>			// getenv()
#include <string.h>			// strlen()
#include <unistd.h> 		// brk(), sbrk()

#include <sys/mman.h> 		// mmap()
#include <sys/resource.h>	// getrusage()
#include <sys/time.h>		// getrusage()
#include <sys/types.h>		// size_t

// for getting vsize
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/mach_traps.h>
#endif


// how is this not standard? 
// might as well go big or go home 
uint64_t u64gcd(uint64_t a, uint64_t b)
{
	uint64_t    r, i;
	while(b!=0) {
		r = a % b;
		a = b;
		b = r;
	}
	return a;
}

// getvsize

ssize_t mc_get_vsize(void)
{
	int64_t 				vsize = -1;
#if defined(__APPLE__)
	task_t 					task = MACH_PORT_NULL;
	struct task_basic_info 	t_info;
	mach_msg_type_number_t 	t_info_count = TASK_BASIC_INFO_COUNT;

	 if (KERN_SUCCESS != task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count))	return -1;
	 vsize  = t_info.virtual_size;	
#endif
	return vsize;
}



// extend breakpoint via sbrk and return pointer
// return NULL on error

inline void *sbrk_morecore(intptr_t incr)
{
	void *core;
	core =  sbrk(incr);
	if(core == (char *)(-1)) return NULL;
	return core;
}

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

// get pages through mmap and return pointer
// return NULL on error 

inline void *mmap_morecore(intptr_t incr)
{
	void *core;

	core = mmap(NULL, incr, (PROT_READ|PROT_WRITE), (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0);
	if(core == MAP_FAILED) return NULL;
	return core;
}

malloc_config mc_config = {0, NULL};

void mc_configure(void)
{
	char *a, *t;
	a = getenv("MC_ALLOCATOR");
	if(a == NULL) {
		mc_config.allocator = MC_ALLOCATOR_DEFAULT;
	}
	if((strcmp(a, "MC_ALLOCATOR_KR")) == 0) {
		mc_config.allocator = MC_ALLOCATOR_KR;
	} else {
		mc_config.allocator = MC_ALLOCATOR_DEFAULT;
	}
	t = getenv("MC_TRACE");
	if(t == NULL) {
		mc_config.trace = NULL;
	} else {
		if((mc_config.trace = fopen(t, "w")) == NULL) {
			perror("fopen()");
		}
	}
}

void *mc_calloc(size_t count, size_t size)
{
	if(mc_config.allocator == 0) {
		mc_configure();
	}	
	if(mc_config.trace != NULL) {
		fprintf(mc_config.trace, "calloc: %lu\n", size);
	}
	switch(mc_config.allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_calloc(count, size);
	}
	return NULL;
}

void *mc_malloc(size_t size)
{
	void *p;

	if(mc_config.allocator == 0) {
		mc_configure();
	}
	if(mc_config.trace != NULL) {
		switch(mc_config.allocator) {
			case MC_ALLOCATOR_KR:
				p = mc_kr_malloc(size);
				break;
		}
		fprintf(mc_config.trace, "malloc(%lu): %p\n", size, p);
		return p;
	}
	switch(mc_config.allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_malloc(size);
	}
	return NULL;
}

static mallstats default_mallstats = {0, 0, 0};
mallstats mc_getmallstats(void)
{	
	if(mc_config.allocator == 0) {
		mc_configure();
	}
	switch(mc_config.allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_getmallstats();
	}

	return default_mallstats;
}

void mc_malloc_stats(void)
{
	if(mc_config.allocator == 0) {
		mc_configure();
	}	
	switch(mc_config.allocator) {
		case  MC_ALLOCATOR_KR:
			mc_kr_malloc_stats();
			return;
	}

	return;
}

void *mc_realloc(void *ptr, size_t size)
{
	if(mc_config.allocator == 0) {
		mc_configure();
	}
	if(mc_config.trace != NULL) {
		fprintf(mc_config.trace, "realloc: %lu\n", size);
	}
	switch(mc_config.allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_malloc(size);
	}
	return NULL;
}
void mc_free(void *ptr)
{
	if(mc_config.trace != NULL) {
		fprintf(mc_config.trace, "free(%p)\n", ptr);
	}	
	switch(mc_config.allocator) {
		case 	MC_ALLOCATOR_KR:
			mc_kr_free(ptr);
			return;
	}
	return;
}
