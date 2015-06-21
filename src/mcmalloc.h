#ifndef MCMALLOC_H
#define MCMALLOC_H

#include <config.h>

#include <stdint.h>  	// uint64_t
#include <sys/types.h>	// size_t

#ifdef __cplusplus
extern "C" {
#endif

// malloc stats
// this gets us utilization and internal/external fragmentation 

struct mc_mallstats {
	size_t 	memoryAllocated;	// user requested bytes by *_malloc
	size_t 	heapAllocated; 		// toatl mapped by *_morecore
	size_t 	allocatedSpace; 	// space used by blocks for memoryAllocated
};

typedef struct mc_mallstats mallstats;


// allocator constants

#define MC_ALLOCATOR_KR	1

// Setup 

// alignment 

// size of struct { } s; below
#define S_SIZE (SIZEOF_VOID_P + SIZEOF_SIZE_T)

// alignment size is 8 bytes unless no uint64_t then 32
#ifdef HAVE_UINT64_T
typedef uint64_t Align;
#define ALIGN_SIZE (SIZEOF_UINT64_T)
#else
typedef uint32_t Align;
#define ALIGN_SIZE SIZEOF_UINT32_T
#endif

// number of alignment blocks in S_SIZE and modulus
#define S_BLOCKS  (S_SIZE / ALIGN_SIZE)  
#define S_MODULUS  (S_SIZE - (ALIGN_SIZE * S_BLOCKS))

// count of alignment elements to pad header if necessary 
#if S_MODULUS == 0
#define ALIGN_COUNT S_BLOCKS
#else
#define ALIGN_COUNT (S_BLOCKS + 1)
#endif

// K&R header
union mc_kr_header {
	struct {
		union mc_kr_header	*ptr;	// next element on free list
		size_t	 			size;	// size of allocation including header
	} s;
	Align x[ALIGN_COUNT]; 			// fix alignment
};

typedef union mc_kr_header mc_kr_Header;

// page size
#define MCMALLOC_PAGE_SIZE 4096

// minimum number of units to allocate
// 1 unit = 1 Header
// This gets 4k pages at a time 

#define NALLOC	((MCMALLOC_PAGE_SIZE) / (ALIGN_COUNT * ALIGN_SIZE))

#ifndef USE_MC_PREFIX
	#define mc_calloc	calloc
	#define mc_malloc 	malloc 
	#define mc_getmallstats 	getmallstats
	#define mc_realloc	realloc
	#define mc_free 	mcfree
#endif

// unit tests

#ifdef HAVE_CUNIT_CUNIT_H
int 	init_mc_utilsSuite(void);
int 	clean_mc_utilsSuite(void);
void 	unit_mc_sbrk_morecore(void);
void 	unit_mc_mmap_morecore(void);

int 	init_mc_kr_mallocSuite(void);
int 	clean_mc_kr_mallocSuite(void);
void 	unit_mc_kr_malloc(void);

int 	init_mc_kr_callocSuite(void);
int 	clean_mc_kr_callocSuite(void);
void 	unit_mc_kr_calloc(void);

int 	init_mc_kr_reallocSuite(void);
int 	clean_mc_kr_reallocSuite(void);
void 	unit_mc_kr_realloc(void);

#endif

// utilities 

static void *sbrk_morecore(intptr_t incr);
static void *mmap_morecore(intptr_t incr);
static void *get_morecore(intptr_t incr);

// K&R C allocator
void *mc_kr_calloc(size_t count, size_t size);
void *mc_kr_malloc(size_t size);
mallstats 	mc_kr_getmallstats(void);
void *mc_kr_realloc(void *ptr, size_t size);
void mc_kr_free(void *ptr);

// C allocator
void *mc_calloc(size_t count, size_t size);
void *mc_malloc(size_t size);
mallstats 	mc_getmallstats(void);
void *mc_realloc(void *ptr, size_t size);
void mc_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
