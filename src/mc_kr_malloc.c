#include <config.h>
#include "mcmalloc.h"

#include <inttypes.h>		// XintY_t
#include <stddef.h>			// NULL
#include <stdio.h> 			// STDERR
#include <string.h>			// strlen()
#include <unistd.h>			// brk(), sbrk()

#include <sys/mman.h>		// mmap()
#include <sys/resource.h>	// getrusage()
#include <sys/time.h>		// getrusage()
#include <sys/types.h>		// size_t

// for getting vsize
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/mach_traps.h>
#endif


// The inital node
mc_kr_Header mc_kr_base;

// the free list
// this will circular linked list
mc_kr_Header *mc_kr_freep = NULL;

// malloc stats
static mallstats mc_kr_mallstats = {0, 0, 0, 0};

mallstats mc_kr_getmallstats(void)
{
	return mc_kr_mallstats;
}

void mc_kr_malloc_stats(void)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "Memory allocation\n");
	fprintf(stderr, "\t allocated blocks \t%lu\n", mc_kr_mallstats.allocationCount);
	fprintf(stderr, "\t memory allocated \t%lu\n", mc_kr_mallstats.memoryAllocated);
	fprintf(stderr, "\t overhead allocated \t%lu\n", mc_kr_mallstats.allocationCount * sizeof(mc_kr_Header));
	fprintf(stderr, "\t space allocated \t%lu\n", mc_kr_mallstats.allocatedSpace);
	fprintf(stderr, "\t heap allocated \t%lu\n", mc_kr_mallstats.heapAllocated);
	fprintf(stderr, "\n");
	fprintf(stderr, "\t internal fragmentation = %f\n", ((double)mc_kr_mallstats.allocatedSpace - mc_kr_mallstats.memoryAllocated) / mc_kr_mallstats.allocatedSpace);
	// (heap - (allocated space + overhead)) / heap
	fprintf(stderr, "\t external fragmentation = %f\n", 
			((double)mc_kr_mallstats.heapAllocated - (mc_kr_mallstats.allocatedSpace + mc_kr_mallstats.allocationCount * sizeof(mc_kr_Header))) / mc_kr_mallstats.heapAllocated);
	fprintf(stderr, "\t overhead = %f\n", (double)mc_kr_mallstats.allocationCount * sizeof(mc_kr_Header) / mc_kr_mallstats.heapAllocated);
	fprintf(stderr, "\t total loss = %f\n", ((double)mc_kr_mallstats.heapAllocated - mc_kr_mallstats.memoryAllocated) / mc_kr_mallstats.heapAllocated);
	return;
}

void mc_kr_malloc_print_header(void *p)
{
	mc_kr_Header *bp;

	bp = (mc_kr_Header *)p - 1;
	fprintf(stderr, "Block at %p\n", p);
	fprintf(stderr, "\t ptr = %p\n", (void *)bp->s.ptr);
	fprintf(stderr, "\t bytes = %ld\n", bp->s.bytes);
	fprintf(stderr, "\t size = %ld\n", bp->s.size);
	return;
}

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
#if defined(HAVE_MMAP)
		munmap(core, core->s.size * sizeof(mc_kr_Header));
#elif !defined(__APPLE__) && defined(HAVE_SBRK)
		sbrk(-(core->s.size * sizeof(mc_kr_Header)));
#endif
		core = next;
	}
	// NB: space may still be allocated but it will no longer be under control of the allocator 
	// so we act like nothing happened
	// reset the free list and base header
	mc_kr_base.s.ptr =  &mc_kr_base;
	mc_kr_base.s.size = 0;
	mc_kr_freep = NULL;
	// reset the stats
	mc_kr_mallstats.allocationCount = 0;
	mc_kr_mallstats.memoryAllocated = 0;
	mc_kr_mallstats.heapAllocated = 0;
	mc_kr_mallstats.allocatedSpace = 0;
	return;
}

void mc_kr_PrintFreelist(void)
{
	mc_kr_Header *current = &mc_kr_base;

	printf("Freelist: \n");
	do {
		printf("[%p, %p): (%lu, %p, %p)\n", (void *)current, (void *)((char *)current + current->s.size * sizeof(mc_kr_Header)), current->s.size, (void *)current->s.ptr, (void *)(current+1));
		current = current->s.ptr;
	} while(current != &mc_kr_base);
}

// mc_kr_morecore()

size_t	mc_kr_pageSize = 0;
size_t 	mc_kr_nalloc = 0;

inline static mc_kr_Header *mc_kr_morecore(size_t nu)
{
	char				*cp;
	mc_kr_Header	*up;

// printf("mc_kr_morecore(%lu), ps = %ld, na = %ld\n", nu, mc_kr_pageSize, mc_kr_nalloc);

	// what we really want is to round up to a multiple of mc_kr_nalloc

	nu = (nu / mc_kr_nalloc + ((nu % mc_kr_nalloc) == 0 ? 0 : 1)) * mc_kr_nalloc;

	// get more space and return NULL if unable
	if((cp = get_morecore(nu*sizeof(mc_kr_Header))) == NULL) return NULL;
	up = (mc_kr_Header *)cp;
	// set the size of the block
	up->s.size = nu;
	up->s.bytes = (nu - 1)*sizeof(mc_kr_Header);
	// update the stats
	// Note, we essentially allocated space that will be freed, so we need to udpate it's stats
	// so free can deincrement properly 
	mc_kr_mallstats.heapAllocated += nu * sizeof(mc_kr_Header);
	mc_kr_mallstats.memoryAllocated += (nu-1)*sizeof(mc_kr_Header);
	mc_kr_mallstats.allocatedSpace += (nu-1) * sizeof(mc_kr_Header);
	mc_kr_mallstats.allocationCount++;
	// This will place the useful space onto the free list
	mc_kr_free((void *)(up+1));
	return mc_kr_freep; 
}

// mc_kr_calloc()

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
	size_t	 		nunits;
	uint64_t 		gcd;

// printf("malloc(%lu)\n", nbytes);
	nunits = (nbytes + sizeof(mc_kr_Header) - 1)/sizeof(mc_kr_Header) + 1;
	// set prevp to the free list and see if it's empty
	if((prevp = mc_kr_freep) == NULL) {
		mc_kr_base.s.ptr = mc_kr_freep = prevp = &mc_kr_base;
		mc_kr_base.s.size = 0;
		// setup the actual page size
		gcd = u64gcd(MCMALLOC_PAGE_SIZE, (ALIGN_COUNT * ALIGN_SIZE));
		mc_kr_pageSize = MCMALLOC_PAGE_SIZE * (ALIGN_COUNT * ALIGN_SIZE) / gcd;
		mc_kr_nalloc = ((mc_kr_pageSize) / (ALIGN_COUNT * ALIGN_SIZE));
	}


	// loop the freelist, incrementing prevp and p
	// with prevp -> p
	for(p = prevp->s.ptr; ; prevp = p, p = p->s.ptr) {
		if(p->s.size >= nunits) { 			// big enough 
			if(p->s.size == nunits) { 		// exactly big enough
				prevp->s.ptr = p->s.ptr;	// cut it out of the list
													// size is already set
				p->s.bytes = nbytes; 		// set the bytes of the new block
			} else {
				// we need to slice out a piece 
				p->s.size -= nunits;		// fix the size of the remainder 
				p += p->s.size;			// go to the slized out block 
				p->s.size = nunits;		// fix the size of the new block
				p->s.bytes = nbytes; 	// fix the bytes of the new block
			}
			// p now points where we want 
			mc_kr_freep = prevp;    // looks like we are doing next fit
			// update the mallstats
			mc_kr_mallstats.memoryAllocated += p->s.bytes;
			// size of block in bytes (not including header)
			mc_kr_mallstats.allocatedSpace += (p->s.size - 1) * sizeof(mc_kr_Header); 
			mc_kr_mallstats.allocationCount++;
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
	mc_kr_Header	*newSpace, *oldSpace;
	mc_kr_Header 	*start, *last, *current;
	void				*newPtr;

	// if NULL then malloc()
	if(ptr == NULL) {
		return mc_kr_malloc(size);
	}
	// set the new size (space for size + 1 for the header)
	newUnits = (size / sizeof(mc_kr_Header)) + (size % sizeof(mc_kr_Header) == 0 ? 0 : 1) + 1;	
	oldSpace = (mc_kr_Header *)ptr - 1;
	// if new size is smaller than old size put the remainder on the free list
	// but only if by more than 2 Headers 
	if(size <= (((mc_kr_Header *)ptr - 1)->s.size - 2) * sizeof(mc_kr_Header)) {
		// compute new size in terms of Header units 
		// put the remainder onto the free list
		
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
		start = &mc_kr_base; 
		last = start;
		current = last->s.ptr;
		while(current != start) {
			if(current == oldSpace + oldSpace->s.size) {
				oldSpace->s.size += current->s.size;	// add the current blocks size
				last->s.ptr = current->s.ptr;				// remove current from the free list
				if(oldSpace->s.size >= newUnits) { 	// we are done
					oldSpace->s.bytes = size;
					return oldSpace+1;
				}
				// we coalesced but need more, start over 
				last = &mc_kr_base;
				current = last->s.ptr;
			}
			last = current;
			current = last->s.ptr;
		}
		// we looped all the way around
		newPtr = mc_kr_malloc(size);
		if(newPtr == NULL) return newPtr;

		memcpy(newPtr, ptr, oldSpace->s.size * (sizeof(mc_kr_Header) - 1));
		mc_kr_free(ptr);
		return newPtr;
	}
	return NULL;
}

void mc_kr_free(void *ap)
{
	mc_kr_Header *bp, *p;

	bp = (mc_kr_Header *)ap - 1; // bp points to the header
// printf("Freeing %ld in %ld space\n", bp->s.bytes, bp->s.size);
	// update the mallstats
	mc_kr_mallstats.memoryAllocated -= bp->s.bytes;
	mc_kr_mallstats.allocatedSpace -= (bp->s.size - 1) * sizeof(mc_kr_Header); // size of block in bytes - header
	mc_kr_mallstats.allocationCount--;

	// loop through the free list while bp is not between p and p->s.ptr (next p)
	for(p = mc_kr_freep; !(p < bp && bp < p->s.ptr); p = p->s.ptr) {
		if(bp == p) return; // it's already on the freelist, probably double free (meconlen)
		if(p < bp && bp < p + p->s.size) return; // the element we are freeing is inside a free block, double free (mconlen)
		// p >= p->s.ptr -> we've reached the end of the list and it points back to the start
		// this assumes that the stack is below everything else, should probably change to look for 0 size element

		// that is {p->s.ptr, p}
		// if bp > p then {p->s.ptr, p, bp}
		// if bp < p->s.ptr then {bp, p->s.ptr, p}
		if(p >= p->s.ptr && (bp > p || bp < p->s.ptr))
			break; 
	}
	if(bp + bp->s.size == p->s.ptr) { // bp is just before p->s.ptr, join bp to p->s.ptr
		bp->s.size += p->s.ptr->s.size; // increase the size of bp
		bp->s.ptr = p->s.ptr->s.ptr; // update the next pointer
	} else {
		bp->s.ptr = p->s.ptr; // point bp to p->s.ptr, inserting bp after p
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
