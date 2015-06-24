#ifndef MC_KR_MALLOC_H
#define MC_KR_MALLOC_H

#include <config.h>

#include <stdint.h>     // uint64_t
#include <sys/types.h>  // size_t

#ifdef __cplusplus
extern "C" {
#endif

// Setup 

// minimum number of units to allocate
// 1 unit = 1 Header
// This gets 4k pages at a time 

// alignment 

// page size
#define MCMALLOC_PAGE_SIZE 4096L

// size of struct { } s; below
#define S_SIZE (SIZEOF_VOID_P + SIZEOF_SIZE_T + SIZEOF_SIZE_T)

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

#define NALLOC ((MCMALLOC_PAGE_SIZE) / (ALIGN_COUNT * ALIGN_SIZE))

// K&R header
union mc_kr_header {
	struct {
		union mc_kr_header	*ptr; // next element on free list
		size_t 					bytes; // bytes requested
		size_t					size; // size of allocation including header
	} s;
	Align x[ALIGN_COUNT];         // fix alignment
};

typedef union mc_kr_header mc_kr_Header;

// Utilities 

void mc_kr_releaseFreeList(void);
void mc_kr_PrintFreelist(void);
void mc_kr_malloc_print_header(void *p);

extern mc_kr_Header mc_kr_base;
extern mc_kr_Header *mc_kr_freep;
extern size_t	mc_kr_pageSize;
extern size_t 	mc_kr_nalloc;

// K&R C allocator
void *mc_kr_calloc(size_t count, size_t size);
void *mc_kr_malloc(size_t size);
mallstats   mc_kr_getmallstats(void);
void mc_kr_malloc_stats(void);
void *mc_kr_realloc(void *ptr, size_t size);
void mc_kr_free(void *ptr);


#ifdef __cplusplus
}
#endif

#endif

