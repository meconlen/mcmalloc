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

// The inital node
static mc_kr_Header mc_kr_base;

// the free list
// this will circular linked list
static mc_kr_Header *mc_kr_freep = NULL;


// This releases the memory in the free list. 
// All memory should be returned to the free list first
//
// used by unit test cleanup 
void mc_kr_releaseFreeList(void)
{
	mc_kr_Header *core = mc_kr_base.s.ptr;
	mc_kr_Header *next;
	while(core != &mc_kr_base) {
		next = core->s.ptr;
#if !defined(__APPLE__) && defined(HAVE_SBRK)
		sbrk(core, -(core->s.size * sizeof(mc_kr_Header));
#elif defined(HAVE_MMAP)
		munmap(core, core->s.size * sizeof(mc_kr_Header));
#endif
		core = next;
	}
	// reset the free list and base header
	mc_kr_base.s.ptr =  &mc_kr_base;
	mc_kr_base.s.size = 0;
	mc_kr_freep = NULL;
	return;
}

void mc_kr_PrintFreelist(void)
{
	mc_kr_Header *current = &mc_kr_base;

	printf("Freelist: \n");
	do {
		printf("%p: (%lu, %p, %p)\n", current, current->s.size, current->s.ptr, current+1);
		current = current->s.ptr;
	} while(current != &mc_kr_base);
}

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
	printf("mc_utilsSuite: start = %" PRIu64 "\n", mc_kr_mallocSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_kr_mallocSuite_vsize);
	return 1;
}

void mc_kr_assertFreelist(size_t size, uint64_t count)
{
	size_t			totalSize = 0;
	uint64_t		freeCount = 0;
	mc_kr_Header 	*current = &mc_kr_base;

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
	char 				*testPtr[4];
	mc_kr_Header 		*current;
	
	size_t		totalSize = 0;
	uint64_t	freeCount = 0;

	void		*ptr[128];
	uint64_t	i;

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
	printf("mc_utilsSuite: start = %" PRIu64 "\n", mc_kr_reallocSuite_vsize);
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
	printf("\n mc_utilsSuite:\n \t start = %" PRIu64 "\n", mc_kr_callocSuite_vsize);
	printf("\t end = %" PRIu64 "\n", mc_get_vsize());
	printf("\t diff = %" PRIu64 "\n", mc_get_vsize() - mc_kr_callocSuite_vsize);
	return 1;
}


void unit_mc_kr_calloc(void)
{
	char *core;
	uint64_t	i;
	core = (char *)mc_kr_calloc(128, 128);
	CU_ASSERT_PTR_NOT_NULL_FATAL(core);
	for(i=0; i<128*128; i++) {
		if(core[i] != 0) break;
	}
	CU_ASSERT(core[i] == 0);
	mc_kr_free(core);
	return;
}

#endif 	/* HAVE_CUNIT_CUNIT_H */

// extend breakpoint via sbrk and return pointer
// return NULL on error

inline static void *sbrk_morecore(intptr_t incr)
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

inline static void *mmap_morecore(intptr_t incr)
{
	void *core;

	core = mmap(NULL, incr, (PROT_READ|PROT_WRITE), (MAP_PRIVATE|MAP_ANONYMOUS), -1, 0);
	if(core == MAP_FAILED) return NULL;
	return core;
}

// malloc stats
static mallstats mc_kr_mallstats = {0, 0, 0};

mallstats mc_kr_getmallstats(void)
{
	return mc_kr_mallstats;
}

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

inline static mc_kr_Header *mc_kr_morecore(unsigned nu)
{
	char 			*cp;
	mc_kr_Header 	*up;

	if(nu < NALLOC) nu = NALLOC;
	// get more space and return NULL if unable
	if((cp = get_morecore(nu*sizeof(mc_kr_Header))) == NULL) return NULL;
	up = (mc_kr_Header *)cp;
	// set the size of the block
	up->s.size = nu;
	// This will place the useful space onto the free list
	mc_kr_free((void *)(up+1));
	return mc_kr_freep; 
}

void *mc_kr_calloc(size_t count, size_t size)
{
	void *core;
	if((core = mc_kr_malloc(count * size)) == NULL) return NULL;
	memset(core, 0, count*size);
	return core;
}

void *mc_kr_malloc(size_t nbytes)
{
	mc_kr_Header 	*p, *prevp;
	uint64_t		nunits;

	nunits = (nbytes + sizeof(mc_kr_Header) - 1)/sizeof(mc_kr_Header) + 1;
	// set prevp to the free list and see if it's empty
	if((prevp = mc_kr_freep) == NULL) {
		mc_kr_base.s.ptr = mc_kr_freep = prevp = &mc_kr_base;
		mc_kr_base.s.size = 0;
	}

	// loop the freelist, incrementing prevp and p
	// with prevp -> p
	for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
		if(p->s.size >= nunits) { // big enough 
			if(p->s.size == nunits) { // exactly big enough
				prevp->s.ptr = p->s.ptr; // cut it out of the list
			} else {
				// we need to slice out a piece 
				p->s.size -= nunits; 	// fix the size 
				p += p->s.size; 		// go to the slized out block 
				p->s.size = nunits;		// fix the size 
			}
			// p now points where we want 
			mc_kr_freep = prevp; 	// looks like we are doing next fit
			return (void *)(p+1); // return the space after the header
		} 
		if(p == mc_kr_freep) { // we wrapped around the list
			// morecore() gets memory then free()s it onto the free list
			// so just keep going!
			if((p = mc_kr_morecore(nunits)) == NULL) return NULL; 
		}
	}

}

void *mc_kr_realloc(void *ptr, size_t size)
{	
	size_t			newUnits;
	mc_kr_Header 	*newSpace, *oldSpace;
	void 			*newPtr;

	// if NULL then malloc()
	if(ptr == NULL) {
		return mc_kr_malloc(size);
	}
	oldSpace = (mc_kr_Header *)ptr - 1;
	// if new size is smaller than old size put the remainder on the free list
	// but only if by more than 2 Headers 
	if(size <= (((mc_kr_Header *)ptr - 1)->s.size - 2) * sizeof(mc_kr_Header)) {
		// compute new size in terms of Header units 
		// set the new size
		// put the remainder onto the free list
		
		newUnits = (size / sizeof(mc_kr_Header)) + (size % sizeof(mc_kr_Header) == 0 ? 0 : 1) + 1;
		if(newUnits < oldSpace->s.size - 2) {
			newSpace = oldSpace + newUnits;
			newSpace->s.size = oldSpace->s.size - newUnits;
			mc_kr_free(newSpace + 1); // this puts it onto the free list
			oldSpace->s.size = newUnits;
			return ptr;
		}
		return ptr;

	}
	// if new size is larger than old size 
	if(size > ((mc_kr_Header *)ptr - 1)->s.size) {
		newPtr = mc_kr_malloc(size);
		if(newPtr == NULL) return newPtr;
		memcpy(newPtr, ptr, oldSpace->s.size * sizeof(mc_kr_Header));
		mc_kr_free(ptr);
	}
	return NULL;
}

void mc_kr_free(void *ap)
{
	mc_kr_Header *bp, *p;

	bp = (mc_kr_Header *)ap - 1; // bp points to the header
	// loop through the free list while bp is not between p and p->s.ptr (next p)
	for(p = mc_kr_freep; !(bp > p && bp < p->s.ptr); p = p->s.ptr) 
		// p >= p->s.ptr -> we've reached the end of the list and it points back to the start
		// that is {p->s.ptr, p}
		// if bp > p then {p->s.ptr, p, bp}
		// if bp < p->s.ptr then {bp, p->s.ptr, p}
		if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break; 
	if(bp + bp->s.size == p->s.ptr) { // bp is just before p, join them
		bp->s.size += p->s.ptr->s.size; // increase the size of bp
		bp->s.ptr = p->s.ptr->s.ptr; // update the next pointer
	} else {
		bp->s.ptr = p->s.ptr; // point bp to p
	}

	if(p + p->s.size == bp) { // p is just before bp, join them
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else {
		p->s.ptr = bp; // p points to bp
	} 
	mc_kr_freep = p;
	return;
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
