#include <config.h>

#include <iostream>

#ifdef HAVE_CUNIT_CUNIT_H
	#include <CUnit/Basic.h>
#endif

#include "mcmalloc.h"

#define USE_DL_PREFIX 1

int main(int argc, char *argv[], char *envp[])
{

#ifdef HAVE_CUNIT_CUNIT_H
	CU_pSuite	utilsSuite = NULL,
				mallocSuite = NULL,
				reallocSuite = NULL,
				callocSuite = NULL;

	if(CUE_SUCCESS != CU_initialize_registry()) goto error1;

	if((utilsSuite = CU_add_suite("mc_utilsSuite", init_mc_utilsSuite, clean_mc_utilsSuite)) == NULL) goto error1;
#if !defined(__APPLE__)
	if((CU_add_test(utilsSuite, "mc_sbrk_morecore", unit_mc_sbrk_morecore)) == NULL) goto error1;
#endif
	if((CU_add_test(utilsSuite, "mc_mmap_morecore", unit_mc_mmap_morecore)) == NULL) goto error1;


	if((mallocSuite = CU_add_suite("mc_kr_mallocSuite", init_mc_kr_mallocSuite, clean_mc_kr_mallocSuite)) == NULL) goto error1;
// test sbrk first
	if((CU_add_test(mallocSuite, "mc_kr_malloc", unit_mc_kr_malloc)) == NULL) goto error1;

	if((callocSuite = CU_add_suite("mc_kr_callocSuite", init_mc_kr_callocSuite, clean_mc_kr_callocSuite)) == NULL) goto error1;
	if((CU_add_test(callocSuite, "mc_kr_calloc", unit_mc_kr_calloc)) == NULL) goto error1;

	if((reallocSuite = CU_add_suite("mc_kr_reallocSuite", init_mc_kr_reallocSuite, clean_mc_kr_reallocSuite)) == NULL) goto error1;
	if((CU_add_test(reallocSuite, "mc_kr_realloc", unit_mc_kr_realloc)) == NULL) goto error1;


	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();

	return(0);
	error1:
	CU_cleanup_registry();
	std::cerr << "Test setup error" << std::endl;

#endif
	return(0);
}
