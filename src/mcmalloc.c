#include <config.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>		// XintY_t
#include <stddef.h> 		// NULL
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

#ifdef HAVE_CUNIT_CUNIT_H
	#include <CUnit/Basic.h>
#endif

#include "mcmalloc.h"

// getvsize

int64_t mc_get_vsize(void)
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


static int mc_allocator = MC_ALLOCATOR_KR;

void *mc_calloc(size_t count, size_t size)
{
	switch(mc_allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_calloc(count, size);
	}
	return NULL;
}
void *mc_malloc(size_t size)
{
	switch(mc_allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_malloc(size);
	}
	return NULL;
}

static mallstats default_mallstats = {0, 0, 0};
mallstats mc_getmallstats(void)
{
	switch(mc_allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_getmallstats();
	}

	return default_mallstats;
}

void *mc_realloc(void *ptr, size_t size)
{
	switch(mc_allocator) {
		case 	MC_ALLOCATOR_KR:
			return mc_kr_malloc(size);
	}
	return NULL;
}
void mc_free(void *ptr)
{
	switch(mc_allocator) {
		case 	MC_ALLOCATOR_KR:
			mc_kr_free(ptr);
	}
	return;
}
