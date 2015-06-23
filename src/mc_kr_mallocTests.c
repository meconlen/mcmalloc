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

	// it won't be set when we start so we compute it again
	size_t 			nallocSize = ((MCMALLOC_PAGE_SIZE * (ALIGN_COUNT * ALIGN_SIZE) / u64gcd(MCMALLOC_PAGE_SIZE, (ALIGN_COUNT * ALIGN_SIZE))) / (ALIGN_COUNT * ALIGN_SIZE));

	CU_ASSERT_PTR_NULL(mc_kr_freep);

	// allocate something that will fill one page (with header)
	testPtr[0] = (char *)mc_kr_malloc((nallocSize - 1) * sizeof(mc_kr_Header));
	CU_ASSERT_FATAL(testPtr[0] != NULL);
	CU_ASSERT(mc_kr_freep == &mc_kr_base);
	CU_ASSERT(mc_kr_freep->s.ptr == &mc_kr_base);

	// free the block and see it's on the freelist
	mc_kr_free(testPtr[0]);
	current = &mc_kr_base;
	CU_ASSERT(current->s.size == 0);
	mc_kr_assertFreelist(nallocSize, 2);

	// Allocate half the block that's now on the free list
	testPtr[0] = (char *)mc_kr_malloc((nallocSize/2 - 1) * sizeof(mc_kr_Header));
	CU_ASSERT_FATAL(testPtr[0] != NULL);
	mc_kr_assertFreelist((nallocSize/2), 2);

	// Allocate the other half of the block 
	testPtr[1] = (char *)mc_kr_malloc((nallocSize/2 - 1) * sizeof(mc_kr_Header));
	CU_ASSERT_FATAL(testPtr[1] != NULL);
	mc_kr_assertFreelist(0, 1);

	// free the first block
	mc_kr_free(testPtr[0]);
	mc_kr_assertFreelist((nallocSize/2), 2);

	// free the second block
	mc_kr_free(testPtr[1]);
	mc_kr_assertFreelist(nallocSize, 2);

	// Allocate group of four for coalescing tests
	for(i=0; i<4; i++) {
		testPtr[i] = (char *)mc_kr_malloc((nallocSize/4 - 1) * sizeof(mc_kr_Header));
		CU_ASSERT_FATAL(testPtr[i] != NULL);
	}
	// Verify the free list is empty
	mc_kr_assertFreelist(0, 1);

	// Allocations are from the end of the free block so they will be 
	// {3, 2, 1, 0} in memory

	// coalescing to the next region 
	mc_kr_free(testPtr[0]);
	mc_kr_free(testPtr[1]);
	mc_kr_assertFreelist((nallocSize/2), 2);

	// coalescing to the previous and next region
	mc_kr_free(testPtr[3]);
	mc_kr_free(testPtr[2]);
	mc_kr_assertFreelist((nallocSize), 2);

	// need to consider that mmap may be allocated top down (linux) or bottom up (OS X)
	// need to test malloc/free in various configurations (though we should have symmetry)
	// allocations from within a block are allocated high to low (top down, back to front)
	
	// Allocate group of four for coalescing tests
	for(i=0; i<4; i++) {
		testPtr[i] = (char *)mc_kr_malloc((nallocSize/4 - 1) * sizeof(mc_kr_Header));
		CU_ASSERT_FATAL(testPtr[i] != NULL);
	}
	// Verify the free list is empty
	mc_kr_assertFreelist(0, 1);

	// coalescing to the previous region
	mc_kr_free(testPtr[3]);
	mc_kr_free(testPtr[2]);
	mc_kr_assertFreelist((nallocSize/2), 2);

	// coalescing to the next and previous region
	mc_kr_free(testPtr[0]);
	mc_kr_free(testPtr[1]);
	mc_kr_assertFreelist((nallocSize), 2);

	// Allocate group of four for coalescing tests
	for(i=0; i<4; i++) {
		testPtr[i] = (char *)mc_kr_malloc((nallocSize/4 - 1) * sizeof(mc_kr_Header));
		CU_ASSERT_FATAL(testPtr[i] != NULL);
	}
	// Verify the free list is empty
	mc_kr_assertFreelist(0, 1);
	for(i=0; i<4; i++) {
		mc_kr_free(testPtr[i]);
	}

	mc_kr_releaseFreeList();


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
	size_t 			nallocSize = ((MCMALLOC_PAGE_SIZE * (ALIGN_COUNT * ALIGN_SIZE) / u64gcd(MCMALLOC_PAGE_SIZE, (ALIGN_COUNT * ALIGN_SIZE))) / (ALIGN_COUNT * ALIGN_SIZE));

	core[0] = mc_kr_malloc(((nallocSize/2) - 1) * sizeof(mc_kr_Header));
	core[1] = mc_kr_realloc(core[0], (nallocSize - 1) * sizeof(mc_kr_Header));
	mc_kr_free(core[1]);
	mc_kr_releaseFreeList();

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

	// allocate a block sized region, verify some internal, no external fragmentation
	core[0] = mc_kr_malloc((mc_kr_nalloc - 1) * sizeof(mc_kr_Header));
	stats = mc_kr_getmallstats();
	CU_ASSERT(stats.allocationCount == 1);
	CU_ASSERT(stats.memoryAllocated == (mc_kr_nalloc - 1) * sizeof(mc_kr_Header));
	if(stats.memoryAllocated != (mc_kr_nalloc - 1) * sizeof(mc_kr_Header)) {
		printf("\nstats.memoryAllocated = %lu, expected %lu\n", stats.memoryAllocated, (mc_kr_nalloc - 1) * sizeof(mc_kr_Header));
	}
	CU_ASSERT(stats.heapAllocated == mc_kr_nalloc * sizeof(mc_kr_Header));
	CU_ASSERT(stats.allocatedSpace == (mc_kr_nalloc - 1) * sizeof(mc_kr_Header));
	if(stats.allocatedSpace != (mc_kr_nalloc - 1) * sizeof(mc_kr_Header)) {
		printf("\nstats.allocatedSpace = %lu, expected = %lu\n", stats.allocatedSpace, mc_kr_nalloc * sizeof(mc_kr_Header));
	}

	// free the block 
	mc_kr_free(core[0]);
	stats = mc_kr_getmallstats();
	CU_ASSERT(stats.allocationCount == 0);
	CU_ASSERT(stats.memoryAllocated == 0);
	if(stats.memoryAllocated != 0) {
		printf("\nstats.memoryAllocated = %lu, expected %lu\n", stats.memoryAllocated, 0L);
	}
	CU_ASSERT(stats.heapAllocated == mc_kr_nalloc * sizeof(mc_kr_Header));
	CU_ASSERT(stats.allocatedSpace == 0);
	if(stats.allocatedSpace != 0) {
		printf("\nstats.allocatedSpace = %lu, expected = %lu\n", stats.allocatedSpace, 0L);
	}


	// allocate a partial block from the region
	core[0] = mc_kr_malloc(((mc_kr_nalloc/2) - 1) * sizeof(mc_kr_Header));
	stats = mc_kr_getmallstats();
	CU_ASSERT(stats.allocationCount == 1);
	CU_ASSERT(stats.memoryAllocated == ((mc_kr_nalloc/2) - 1) * sizeof(mc_kr_Header));
	// shouldn't need anymore heap yet 
	CU_ASSERT(stats.heapAllocated == mc_kr_nalloc * sizeof(mc_kr_Header));
	// allocator should be able to allocate exactly the right sized block since 
	// the request is terms of mc_kr_nalloc
	CU_ASSERT(stats.allocatedSpace == ((mc_kr_nalloc/2) - 1 ) * sizeof(mc_kr_Header));

	// allocate the rest of the block from the region
	core[1] = mc_kr_malloc( ((mc_kr_nalloc/2) - 1) * sizeof(mc_kr_Header) );
	stats = mc_kr_getmallstats();
	CU_ASSERT(stats.allocationCount == 2);
	CU_ASSERT(stats.memoryAllocated == ((mc_kr_nalloc) - 2) * sizeof(mc_kr_Header));
	CU_ASSERT(stats.heapAllocated == mc_kr_nalloc * sizeof(mc_kr_Header));
	CU_ASSERT(stats.allocatedSpace == ((mc_kr_nalloc) - 2) * sizeof(mc_kr_Header));

	mc_kr_free(core[0]);
	mc_kr_free(core[1]);

	// start over 
	mc_kr_releaseFreeList();

	// allocate a single byte
	core[0] = mc_kr_malloc(1);
	stats = mc_kr_getmallstats();
	CU_ASSERT(stats.allocationCount == 1);
	CU_ASSERT(stats.memoryAllocated == 1);
	CU_ASSERT(stats.heapAllocated == mc_kr_nalloc * sizeof(mc_kr_Header));
	// is it one header size or 
	CU_ASSERT(stats.allocatedSpace == sizeof(mc_kr_Header));
	if(stats.allocatedSpace != sizeof(mc_kr_Header)) {
		printf("stats.allocatedSpace = %ld, expected %ld\n", stats.allocatedSpace, sizeof(mc_kr_Header));
	}
	// check that we allocate some multiple of page size (because apparently we weren't)
	CU_ASSERT(stats.heapAllocated % MCMALLOC_PAGE_SIZE == 0);
	if(stats.heapAllocated % MCMALLOC_PAGE_SIZE != 0) {
		printf("\nexpected a multiple of %ld, got %ld = %ld (mod %ld)\n", MCMALLOC_PAGE_SIZE, stats.heapAllocated, stats.heapAllocated % MCMALLOC_PAGE_SIZE, MCMALLOC_PAGE_SIZE);
		printf("\t mc_kr_nalloc = %lu\n", mc_kr_nalloc);
		printf("\t ALIGN_COUNT = %d\n", ALIGN_COUNT);

	}
	mc_kr_free(core[0]);

	mc_kr_releaseFreeList();

	return;
}

#endif   /* HAVE_CUNIT_CUNIT_H */
