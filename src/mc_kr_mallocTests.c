#include <config.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>      // XintY_t
#include <stddef.h>     // NULL
#include <string.h>        // strlen()
#include <unistd.h>     // brk(), sbrk()

#include <sys/mman.h>      // mmap()
#include <sys/resource.h>  // getrusage()
#include <sys/time.h>      // getrusage()
#include <sys/types.h>     // size_t

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

#ifdef HAVE_CUNIT_CUNIT_H

static uint64_t mc_utilsSuite_vsize;

int init_mc_utilsSuite(void)
{
	mc_utilsSuite_vsize = mc_get_vsize();
	return 0;
}

int clean_mc_utilsSuite(void)
{
	if(mc_utilsSuite_vsize == mc_get_vsize()) {
		return 0;
	}
	printf("mc_utilsSuite: start = %" PRIu64 "\n", mc_utilsSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_utilsSuite_vsize);
	return 1;
}


void unit_mc_sbrk_morecore(void)
{
	char *curBrk = NULL, *newBrk = NULL;

	curBrk = (char *)sbrk(0);
	sbrk_morecore(4096);
	newBrk = (char *)sbrk(0);
	CU_ASSERT_PTR_NOT_NULL(newBrk);
	CU_ASSERT(newBrk - curBrk == 4096);
// the following is borked on OS X because 
// Apple decided so
#if !defined(__APPLE__)
	sbrk(-4096);
	newBrk = (char *)sbrk(0);
	CU_ASSERT(newBrk == curBrk);
#endif
	return;
}

void unit_mc_mmap_morecore(void)
{
	void *core = NULL;

	core = mmap_morecore(4096);
	CU_ASSERT_PTR_NOT_NULL(core);
	munmap(core, 4096);
	return;
}

static uint64_t mc_kr_mallocSuite_vsize;

int init_mc_kr_mallocSuite(void)
{
	mc_kr_mallocSuite_vsize = mc_get_vsize();
	return 0;
}

int clean_mc_kr_mallocSuite(void)
{
	mc_kr_releaseFreeList();
	if(mc_kr_mallocSuite_vsize == mc_get_vsize()) {
		return 0;
	}
	printf("mc_kr_mallocSuite: start = %" PRIu64 "\n", mc_kr_mallocSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_kr_mallocSuite_vsize);
	return 1;
}

void mc_kr_assertFreelist(size_t size, uint64_t count)
{
	size_t         totalSize = 0;
	uint64_t    freeCount = 0;
	mc_kr_Header   *current = &mc_kr_base;

	do {
		totalSize += current->s.size; 
		freeCount++;
		current = current->s.ptr;
	} while(current != &mc_kr_base);
	CU_ASSERT(totalSize == size);
	CU_ASSERT(freeCount == count);      
	return;
}

void unit_mc_kr_malloc(void)
{
	char				*testPtr[4];
	mc_kr_Header	*current;
	
	size_t			totalSize = 0;
	uint64_t			freeCount = 0;

	void				*ptr[128];
	uint64_t			i;

	CU_ASSERT_PTR_NULL(mc_kr_freep);

	// allocate something that will fill one page (with header)
	testPtr[0] = (char *)mc_kr_malloc((NALLOC - 1) * sizeof(mc_kr_Header));
	CU_ASSERT_FATAL(testPtr[0] != NULL);
	CU_ASSERT(mc_kr_freep == &mc_kr_base);
	CU_ASSERT(mc_kr_freep->s.ptr == &mc_kr_base);

	// free the block and see it's on the freelist
	mc_kr_free(testPtr[0]);
	current = &mc_kr_base;
	CU_ASSERT(current->s.size == 0);
	mc_kr_assertFreelist(NALLOC, 2);

	// Allocate half the block that's now on the free list
	testPtr[0] = (char *)mc_kr_malloc((NALLOC/2 - 1) * sizeof(mc_kr_Header));
	CU_ASSERT_FATAL(testPtr[0] != NULL);
	mc_kr_assertFreelist((NALLOC/2), 2);

	// Allocate the other half of the block 
	testPtr[1] = (char *)mc_kr_malloc((NALLOC/2 - 1) * sizeof(mc_kr_Header));
	CU_ASSERT_FATAL(testPtr[1] != NULL);
	mc_kr_assertFreelist(0, 1);

	// free the first block
	mc_kr_free(testPtr[0]);
	mc_kr_assertFreelist((NALLOC/2), 2);

	// free the second block
	mc_kr_free(testPtr[1]);
	mc_kr_assertFreelist(NALLOC, 2);

	// Allocate group of four for coalescing tests
	for(i=0; i<4; i++) {
		testPtr[i] = (char *)mc_kr_malloc((NALLOC/4 - 1) * sizeof(mc_kr_Header));
		CU_ASSERT_FATAL(testPtr[i] != NULL);
	}
	// Verify the free list is empty
	mc_kr_assertFreelist(0, 1);

	// Allocations are from the end of the free block so they will be 
	// {3, 2, 1, 0} in memory

	// coalescing to the next region 
	mc_kr_free(testPtr[0]);
	mc_kr_free(testPtr[1]);
	mc_kr_assertFreelist((NALLOC/2), 2);

	// coalescing to the previous and next region
	mc_kr_free(testPtr[3]);
	mc_kr_free(testPtr[2]);
	mc_kr_assertFreelist((NALLOC), 2);

	// Allocate group of four for coalescing tests
	for(i=0; i<4; i++) {
		testPtr[i] = (char *)mc_kr_malloc((NALLOC/4 - 1) * sizeof(mc_kr_Header));
		CU_ASSERT_FATAL(testPtr[i] != NULL);
	}
	// Verify the free list is empty
	mc_kr_assertFreelist(0, 1);

	// coalescing to the previous region
	mc_kr_free(testPtr[3]);
	mc_kr_free(testPtr[2]);
	mc_kr_assertFreelist((NALLOC/2), 2);

	// coalescing to the next and previous region
	mc_kr_free(testPtr[0]);
	mc_kr_free(testPtr[1]);
	mc_kr_assertFreelist((NALLOC), 2);

	// break it up
	for(i=0; i<128; i++) ptr[i] = mc_kr_malloc(i+1);
	for(i=0; i<128; i++) mc_kr_free(ptr[i]); 

	return;
}

static uint64_t mc_kr_reallocSuite_vsize;

int init_mc_kr_reallocSuite(void)
{
	mc_kr_reallocSuite_vsize = mc_get_vsize();
	return 0;
}

int clean_mc_kr_reallocSuite(void)
{
	mc_kr_releaseFreeList();
	if(mc_kr_reallocSuite_vsize == mc_get_vsize()) {
		return 0;
	}
	printf("mc_kr_reallocSuite: start = %" PRIu64 "\n", mc_kr_reallocSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_kr_reallocSuite_vsize);
	return 1;
}

// allocate a large block and free, then allocate half and realloc the rest 
// allocate a large block and free, then allocate four small blocks, then realloc to the large block
// allocate a large block then realloc to something larger 
// allocat ea large block and free, then allocate half, the relloc to something larger than the the block + free list 

void unit_mc_kr_realloc(void)
{
	void *core[4], *oldCore;

	core[0] = mc_kr_malloc(((NALLOC) - 1) * sizeof(mc_kr_Header));
	oldCore = core[0];
	core[0] = mc_kr_realloc(core[0], ((NALLOC/2) - 1) * sizeof(mc_kr_Header));
	CU_ASSERT(core[0] == oldCore);
	mc_kr_free(core[0]);
	return;
}

static uint64_t mc_kr_callocSuite_vsize;

int init_mc_kr_callocSuite(void)
{
	mc_kr_callocSuite_vsize = mc_get_vsize();
	return 0;
}

int clean_mc_kr_callocSuite(void)
{
	mc_kr_releaseFreeList();
	if(mc_kr_callocSuite_vsize == mc_get_vsize()) {
		return 0;
	}
	printf("\n mc_kr_callocSuite:\n \t start = %" PRIu64 "\n", mc_kr_callocSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_kr_callocSuite_vsize);
	return 1;
}


void unit_mc_kr_calloc(void)
{
	char *core;
	uint64_t i;
	core = (char *)mc_kr_calloc(128, 128);
	CU_ASSERT_PTR_NOT_NULL_FATAL(core);
	for(i=0; i<128*128; i++) {
		if(core[i] != 0) break;
	}
	CU_ASSERT(core[i] == 0);
	mc_kr_free(core);
	return;
}

// mallstats

int   init_mc_kr_mallstatsSuite(void)
{
	mallstats   current;
	current = mc_kr_getmallstats();
	if(current.memoryAllocated != 0 || current.heapAllocated != 0 || current.allocatedSpace !=0) return -1;
	return 0;
}

int   clean_mc_kr_mallstatsSuite(void)
{
 	mc_kr_releaseFreeList();
	if(mc_kr_callocSuite_vsize == mc_get_vsize()) {
		return 0;
	}
	printf("\n mc_kr_mallstatsSuite:\n \t start = %" PRIu64 "\n", mc_kr_callocSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_kr_callocSuite_vsize);
	return 1;
}

void  unit_mc_kr_mallstats(void)
{
	void 			*core[4];
	mallstats 	stats;

	// allocate a block, verify some internal, no external fragmentation
	core[0] = mc_kr_malloc((NALLOC - 1) * sizeof(mc_kr_Header));
	stats = mc_kr_getmallstats();
	CU_ASSERT(stats.memoryAllocated == (NALLOC - 1) * sizeof(mc_kr_Header));
	if(stats.memoryAllocated != (NALLOC - 1) * sizeof(mc_kr_Header)) {
		printf("\nstats.memoryAllocated = %lu, expected %lu\n", stats.memoryAllocated, (NALLOC - 1) * sizeof(mc_kr_Header));
	}
	CU_ASSERT(stats.heapAllocated == NALLOC * sizeof(mc_kr_Header));
	CU_ASSERT(stats.allocatedSpace == (NALLOC - 1) * sizeof(mc_kr_Header));
	if(stats.allocatedSpace != (NALLOC - 1) * sizeof(mc_kr_Header)) {
		printf("\nstats.allocatedSpace = %lu, expected = %lu\n", stats.allocatedSpace, (NALLOC - 1)*sizeof(mc_kr_Header));
	}
	mc_kr_free(core[0]);
	CU_ASSERT(stats.memoryAllocated == 0);
	if(stats.memoryAllocated != 0) {
		printf("\nstats.memoryAllocated = %lu, expected %lu\n", stats.memoryAllocated, 0L);
	}
	CU_ASSERT(stats.heapAllocated == NALLOC * sizeof(mc_kr_Header));
	CU_ASSERT(stats.allocatedSpace == 0);
	if(stats.allocatedSpace != 0) {
		printf("\nstats.allocatedSpace = %lu, expected = %lu\n", stats.allocatedSpace, 0L);
	}
	return;
}

#endif   /* HAVE_CUNIT_CUNIT_H */
