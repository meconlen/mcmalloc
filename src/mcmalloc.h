#ifndef MCMALLOC_H
#define MCMALLOC_H

#include <config.h>

#define __STDC_FORMAT_MACROS
#define _BSD_SOURCE 1

// we need to use __THROW on glibc to match their headers 
// so if we aren't there define these (thanks tcmalloc)
#ifndef __THROW
#define __THROW
#define __attribute_malloc__
#define __attribute_warn_unused_result__
#define __wur
#endif

#include <stdint.h>     // uint64_t
#include <stdio.h>		// FILE*
#include <sys/types.h>  // size_t

#ifdef __cplusplus
extern "C" {
#endif

// malloc stats
// this gets us utilization and internal/external fragmentation 

struct mc_mallstats {
	size_t 	allocationCount; 	// count of the current number of allocations (allows computation of overhead)
	size_t   memoryAllocated;	// bytes requested and returned  by *_malloc
	size_t   heapAllocated;		// toatl mapped by *_morecore, measures interal + external fragmentation
	size_t   allocatedSpace;	// actual bytes allocated by *_malloc, measures internal fragmentation
};

typedef struct mc_mallstats mallstats;


// allocator constants

#define MC_ALLOCATOR_KR 1

// change the default here
#define MC_ALLOCATOR_DEFAULT MC_ALLOCATOR_KR

#ifndef USE_MC_PREFIX
	#define mc_calloc			calloc
	#define mc_malloc 			malloc 
	#define mc_getmallstats		getmallstats
	#define mc_malloc_stats		malloc_stats
	#define mc_realloc			realloc
	#define mc_free				free
#endif

// utilities the allocators call

uint64_t u64gcd(uint64_t a, uint64_t b);

void *sbrk_morecore(intptr_t incr);
void *mmap_morecore(intptr_t incr);

// OS X only has 4 MB of traditional sbrk space and is actually emulated
// http://www.opensource.apple.com/source/Libc/Libc-763.12/emulated/brk.c
// so we need to use mmap()
#if defined(HAVE_MMAP)
#define get_morecore mmap_morecore
#elif !defined(__APPLE__) && defined(HAVE_SBRK)
#define get_morecore sbrk_morecore
#else
#error No way to get memory
#endif

#ifdef __cplusplus
}
#endif

// Include headers for each allocator here

#include "mc_kr_malloc.h"



// Include headers for each allocator's unit tests here

#include "mc_kr_mallocTests.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration Structure 


struct mc_malloc_config {
	unsigned long 	allocator; 	// MC_ALLOCATOR
	FILE 			*trace; 	// MC_TRACE
};

typedef struct mc_malloc_config malloc_config;
extern malloc_config mc_config;

// utilities 

ssize_t mc_get_vsize(void);


// C allocator
void *mc_calloc(size_t count, size_t size) __THROW __attribute_malloc__ __wur;
void *mc_malloc(size_t size) __THROW __attribute_malloc__ __wur;
mallstats   mc_getmallstats(void);
void mc_malloc_stats(void);
void *mc_realloc(void *ptr, size_t size)  __THROW __attribute_warn_unused_result__;
void mc_free(void *ptr)  __THROW;

#ifdef __cplusplus
}
#endif

#endif
