noinst_LIBRARIES = libmcmalloc.a libmcmalloctest.a
libmcmalloctest_a_SOURCES = mcmalloc.c mc_kr_malloc.c
libmcmalloctest_a_CPPFLAGS = -DUSE_MC_PREFIX
libmcmalloc_a_SOURCES = mcmalloc.c mc_kr_malloc.c
bin_PROGRAMS = test
test_SOURCES = test.cpp mc_kr_mallocTests.c
test_LDADD = libmcmalloctest.a