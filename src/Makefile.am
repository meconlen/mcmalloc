noinst_LIBRARIES = libmcmalloctest.a
lib_LIBRARIES = libmcmalloc.a
libmcmalloctest_a_SOURCES = mcmalloc.c mc_kr_malloc.c
libmcmalloctest_a_CPPFLAGS = -DUSE_MC_PREFIX
libmcmalloc_a_SOURCES = mcmalloc.c mc_kr_malloc.c
libmcmalloctest_a_CFLAGS = $(AM_CFLAGS)
libmcmalloc_a_CFLAGS = $(AM_CFLAGS)

lib_LTLIBRARIES = libmcmalloc.la
libmcmalloc_la_SOURCES = mcmalloc.c mc_kr_malloc.c
include_HEADERS = mcmalloc.h

EXTRA_PROGRAMS = test mc_perf system_perf pagesize
test_SOURCES = test.cpp mcmallocTests.cpp mc_kr_mallocTests.cpp
test_CPPFLAGS = -DUSE_MC_PREFIX
test_LDADD = libmcmalloctest.a
mc_perf_SOURCES = perf.cpp
mc_perf_LDADD = libmcmalloc.a
mc_perf_CPPFLAGS = -DHAVE_MALLOC_STATS
system_perf_SOURCES = perf.cpp

pagesize_SOURCES = pagesize.cpp