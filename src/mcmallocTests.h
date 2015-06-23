#ifndef MCMALLOCTESTS_H
#define MCMALLOCTESTS_H

#include "config.h"
#include "mcmallocTests.h"

#ifdef HAVE_CUNIT_CUNIT_H

int   init_mc_utilsSuite(void);
int   clean_mc_utilsSuite(void);
void  unit_mc_sbrk_morecore(void);
void  unit_mc_mmap_morecore(void);
void  unit_u64gcd(void);

#endif
#endif
