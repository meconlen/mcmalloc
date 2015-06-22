#include <config.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h> 

#ifdef HAVE_CUNIT_CUNIT_H
   #include <CUnit/Basic.h>
#endif

#include "mcmalloc.h"

#include "mcmallocTests.h"

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

void unit_u64gcd(void)
{
   uint64_t    x;

   x = u64gcd(4096, 24);
   CU_ASSERT(x == 8);
   if(x != 8) {
      printf("gcd(4096, 24) = %" PRIu64 "\n", x);
   }
   return;
}