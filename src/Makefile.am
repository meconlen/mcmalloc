noinst_LIBRARIES = libmcmalloc.a libmcmalloctest.a
libmcmalloctest_a_SOURCES = mcmalloc.c
libmcmalloctest_a_CPPFLAGS = -DUSE_MC_PREFIX
libmcmalloc_a_SOURCES = mcmalloc.c
bin_PROGRAMS = test
test_SOURCES = test.cpp
test_LDADD = libmcmalloctest.a