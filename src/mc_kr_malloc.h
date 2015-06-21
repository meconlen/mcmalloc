#ifndef MC_KR_MALLOC_H
#define MC_KR_MALLOC_H

#include <config.h>

#include <stdint.h>     // uint64_t
#include <sys/types.h>  // size_t

#ifdef __cplusplus
extern "C" {
#endif

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
      union mc_kr_header   *ptr; // next element on free list
      size_t            size; // size of allocation including header
   } s;
   Align x[ALIGN_COUNT];         // fix alignment
};

typedef union mc_kr_header mc_kr_Header;

// Utilities 

void mc_kr_releaseFreeList(void);

extern mc_kr_Header mc_kr_base;
extern mc_kr_Header *mc_kr_freep;

// K&R C allocator
void *mc_kr_calloc(size_t count, size_t size);
void *mc_kr_malloc(size_t size);
mallstats   mc_kr_getmallstats(void);
void *mc_kr_realloc(void *ptr, size_t size);
void mc_kr_free(void *ptr);


#ifdef __cplusplus
}
#endif

#endif

