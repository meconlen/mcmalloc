#ifndef MC_KR_MALLOCTESTS_H
#define MC_KR_MALLOCTESTS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CUNIT_CUNIT_H

int	init_mc_kr_mallocSuite(void);
int	clean_mc_kr_mallocSuite(void);
void	unit_mc_kr_malloc(void);

int	init_mc_kr_callocSuite(void);
int	clean_mc_kr_callocSuite(void);
void	unit_mc_kr_calloc(void);

int	init_mc_kr_reallocSuite(void);
int	clean_mc_kr_reallocSuite(void);
void	unit_mc_kr_realloc(void);

int	init_mc_kr_mallstatsSuite(void);
int	clean_mc_kr_mallstatsSuite(void);
void	unit_mc_kr_mallstats(void);
#endif

#ifdef __cplusplus
}
#endif


#endif /* MC_KR_MALLOCTESTS_H */
