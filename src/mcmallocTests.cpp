#include <config.h>

#include "mcmalloc.h"

#include <iostream>

#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/mman.h> 
#include <sys/stat.h>

#ifdef HAVE_CUNIT_CUNIT_H
	#include <CUnit/Basic.h>
#endif

using namespace std;

static ssize_t mc_utilsSuite_vsize;

#ifdef HAVE_CUNIT_CUNIT_H
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
	cout << "mc_utilsSuite: start = " << mc_utilsSuite_vsize << endl;
	cout << "\t end = " << mc_get_vsize() << endl;
	cout << "\t diff = " <<  mc_get_vsize() - mc_utilsSuite_vsize << endl;
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

void unit_u64gcd(void)
{
	uint64_t    x;

	x = u64gcd(4096, 24);
	CU_ASSERT(x == 8);
	if(x != 8) {
		cout << "gcd(4096, 24) = " << x << endl;
	}
	return;
}

int init_mc_trace(void) 
{
	int rc;
	if((rc = access("trace.out", F_OK)) == 0) {
		if((rc = unlink("trace.out")) != 0) {
			perror("unlink(\"trace.out\")\n");
			return 1;
		}
	}
	return 0;
}

int clean_mc_trace(void)
{
	int rc;
	return 0;
	if((rc = access("trace.out", F_OK)) == 0) {
		if((rc = unlink("trace.out")) != 0) {
			perror("unlink(\"trace.out\")\n");
			return 1;
		}
	}
	return 0;
}

void unit_mc_trace(void)
{
	void 	*core;
	char 	*traceData, *free, *closeP;
	int 	trace, rc;

	struct stat 	traceStat;
	if((rc = setenv("MC_TRACE", "trace.out", 1)) != 0) {
		CU_ASSERT(rc == 0);
		return;
	}
	if((rc = setenv("MC_ALLOCATOR", "MC_ALLOCATOR_KR", 1)) != 0) {
		CU_ASSERT(rc == 0);
		return;
	}
	core = mc_malloc(1024);
	CU_ASSERT_PTR_NOT_NULL(core);
	mc_free(core);
	mc_kr_releaseFreeList();
	fflush(mc_config.trace);
	if((trace = open("trace.out", O_RDONLY)) == -1) {
		CU_ASSERT(trace != -1);
		return;
	}
	if((rc = fstat(trace, &traceStat)) == -1) {
		CU_ASSERT(rc != -1);
		goto end0;
		return;
	}
	if((traceData = (char *)mmap(NULL, traceStat.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, trace, 0)) == MAP_FAILED) {
		CU_ASSERT(traceData != MAP_FAILED);
		perror("mmap(): ");
		close(trace);
		return;
	}	
	CU_ASSERT(traceStat.st_size > 24);
	if(traceStat.st_size <= 24) {
		printf("traceStat.st_size = %lld\n", traceStat.st_size);
		goto end1;
	}
// do we start with malloc(1024): 
	rc = memcmp(traceData, "malloc(1024): ", 14);
	CU_ASSERT(rc == 0);
// is there a "free("
	free = strnstr(traceData, "free(", traceStat.st_size);
	CU_ASSERT_PTR_NOT_NULL(free);
	if(free == NULL) goto end1;
// figure out how many bytes are in the () and compare the two pointers 
	closeP = strnstr(free, ")", traceStat.st_size - (free - traceData));
	rc = strncmp(traceData + 14, free + 5, closeP - free - 5);
	CU_ASSERT(rc == 0);

end1:
	munmap(traceData, traceStat.st_size);
end0:
	close(trace);
	return;
}


#endif

