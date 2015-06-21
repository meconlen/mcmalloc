#ifndef MCMALLOC_H
#define MCMALLOC_H

#include <config.h>

#include <stdint.h>     // uint64_t
#include <sys/types.h>  // size_t

#ifdef __cplusplus
extern "C" {
#endif

// malloc stats
// this gets us utilization and internal/external fragmentation 

struct mc_mallstats {
	size_t   memoryAllocated;	// bytes returned by *_malloc (not the bytes requested)
	size_t   heapAllocated;		// toatl mapped by *_morecore, measures interal + external fragmentation
	size_t   allocatedSpace;	// actual bytes allocated by *_malloc, measures internal fragmentation
};

typedef struct mc_mallstats mallstats;


// allocator constants

#define MC_ALLOCATOR_KR 1

// page size
#define MCMALLOC_PAGE_SIZE 4096

// minimum number of units to allocate
// 1 unit = 1 Header
// This gets 4k pages at a time 

#define NALLOC ((MCMALLOC_PAGE_SIZE) / (ALIGN_COUNT * ALIGN_SIZE))

#ifndef USE_MC_PREFIX
	#define mc_calloc calloc
	#define mc_malloc    malloc 
	#define mc_getmallstats    getmallstats
	#define mc_realloc   realloc
	#define mc_free   mcfree
#endif

// utilities the allocators call

void *sbrk_morecore(intptr_t incr);
void *mmap_morecore(intptr_t incr);

// OS X only has 4 MB of traditional sbrk space and is actually emulated
// http://www.opensource.apple.com/source/Libc/Libc-763.12/emulated/brk.c
// so we need to use mmap()
#if !defined(__APPLE__) && defined(HAVE_SBRK)
#define get_morecore sbrk_morecore
#elif defined(HAVE_MMAP)
#define get_morecore mmap_morecore
#elif
#error No way to get memory
#endif

#ifdef __cplusplus
}
#endif

// Include headers for each allocator here

#include "mc_kr_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

// unit tests

#ifdef HAVE_CUNIT_CUNIT_H
int   init_mc_utilsSuite(void);
int   clean_mc_utilsSuite(void);
void  unit_mc_sbrk_morecore(void);
void  unit_mc_mmap_morecore(void);

#endif

#ifdef __cplusplus
}
#endif

// Include headers for each allocator's unit tests here

#include "mc_kr_mallocTests.h"

#ifdef __cplusplus
extern "C" {
#endif

// utilities 

int64_t mc_get_vsize(void);


// C allocator
void *mc_calloc(size_t count, size_t size);
void *mc_malloc(size_t size);
mallstats   mc_getmallstats(void);
void *mc_realloc(void *ptr, size_t size);
void mc_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
