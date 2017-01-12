AM_CPPFLAGS = -I$(top_srcdir)/include @hwloc_inc@
AM_CXXFLAGS = -std=c++11
AM_CFLAGS = -std=gnu11
AM_LDFLAGS = @pthread_cflags@


SSMALLOC=SSMalloc/ssmalloc.c
SSMALLOCFLAGS=-I$(top_srcdir)/SSMalloc/include-x86_64

JEMALLOC=jemalloc/src/arena.c jemalloc/src/base.c jemalloc/src/chunk.c        \
         jemalloc/src/ctl.c  jemalloc/src/hash.c jemalloc/src/huge.c          \
         jemalloc/src/mutex.c jemalloc/src/pages.c jemalloc/src/tsd.c      \
         jemalloc/src/prof.c jemalloc/src/stats.c jemalloc/src/ticker.c       \
         jemalloc/src/util.c jemalloc/src/atomic.c jemalloc/src/witness.c     \
         jemalloc/src/bitmap.c jemalloc/src/ckh.c jemalloc/src/extent.c       \
         jemalloc/src/jemalloc.c jemalloc/src/mb.c jemalloc/src/quarantine.c  \
         jemalloc/src/nstime.c jemalloc/src/prng.c jemalloc/src/rtree.c       \
         jemalloc/src/tcache.c jemalloc/src/valgrind.c                        \
         jemalloc/src/chunk_dss.c jemalloc/src/chunk_mmap.c
# not built:  jemalloc/src/witness.c

JEMALLOCFLAGS=-Wall -Wsign-compare -pipe -g3 -fvisibility=hidden -funroll-loops -c -D_GNU_SOURCE -D_REENTRANT -I$(top_srcdir)/jemalloc/include

bin_PROGRAMS = qrate_folly
qrate_folly_SOURCES = src/printme.c src/qrate.cc folly/folly/detail/Futex.cpp
qrate_folly_CPPFLAGS = -DQUEUE_METHOD=FOLLY_QUEUE -I$(top_srcdir)/folly ${AM_CPPFLAGS}

bin_PROGRAMS += qrate_mc
qrate_mc_SOURCES = src/printme.c src/qrate.cc
qrate_mc_CPPFLAGS = -DQUEUE_METHOD=MOODY_CAMEL_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += qrate_cloudius
qrate_cloudius_SOURCES = src/printme.c src/qrate.cc
qrate_cloudius_CPPFLAGS = -DQUEUE_METHOD=CLOUDIUS_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += qrate_natsys
qrate_natsys_SOURCES = src/printme.c src/qrate.cc
qrate_natsys_CPPFLAGS = -DQUEUE_METHOD=NATSYS_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += qrate_vyukov
qrate_vyukov_SOURCES = src/printme.c src/qrate.cc
qrate_vyukov_CPPFLAGS = -DQUEUE_METHOD=VYUKOV_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += qrate_tbb
qrate_tbb_SOURCES = src/printme.c src/qrate.cc \
                    concurrentqueue/benchmarks/tbb/tbb_misc.cpp \
                    concurrentqueue/benchmarks/tbb/cache_aligned_allocator.cpp \
                    concurrentqueue/benchmarks/tbb/dynamic_link.cpp
qrate_tbb_CPPFLAGS = -DQUEUE_METHOD=TBB_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_vyukov_tbbmalloc
alloc_rate_vyukov_tbbmalloc_SOURCES = src/printme.c src/alloc_rate.cc \
                    concurrentqueue/benchmarks/tbb/tbb_misc.cpp \
                    concurrentqueue/benchmarks/tbb/cache_aligned_allocator.cpp \
                    concurrentqueue/benchmarks/tbb/dynamic_link.cpp

alloc_rate_vyukov_tbbmalloc_CPPFLAGS = -DALLOC_METHOD=TBB_ALLOC -DQUEUE_METHOD=VYUKOV_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_mc_tbbmalloc
alloc_rate_mc_tbbmalloc_SOURCES = src/printme.c src/alloc_rate.cc \
                    concurrentqueue/benchmarks/tbb/tbb_misc.cpp \
                    concurrentqueue/benchmarks/tbb/cache_aligned_allocator.cpp \
                    concurrentqueue/benchmarks/tbb/dynamic_link.cpp
alloc_rate_mc_tbbmalloc_CPPFLAGS = -DALLOC_METHOD=TBB_ALLOC -DQUEUE_METHOD=MOODY_CAMEL_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}


bin_PROGRAMS += alloc_rate_vyukov_ssmalloc
alloc_rate_vyukov_ssmalloc_SOURCES = src/printme.c src/alloc_rate.cc ${SSMALLOC}
alloc_rate_vyukov_ssmalloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=VYUKOV_QUEUE ${AM_CPPFLAGS} ${SSMALLOCFLAGS}

bin_PROGRAMS += alloc_rate_mc_ssmalloc
alloc_rate_mc_ssmalloc_SOURCES = src/printme.c src/alloc_rate.cc ${SSMALLOC}
alloc_rate_mc_ssmalloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=MOODY_CAMEL_QUEUE ${AM_CPPFLAGS} ${SSMALLOCFLAGS}

bin_PROGRAMS += alloc_rate_vyukov_jemalloc
alloc_rate_vyukov_jemalloc_SOURCES = src/printme.c src/alloc_rate.cc ${JEMALLOC}
alloc_rate_vyukov_jemalloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=VYUKOV_QUEUE ${AM_CPPFLAGS} ${JEMALLOCFLAGS}

bin_PROGRAMS += alloc_rate_mc_jemalloc
alloc_rate_mc_jemalloc_SOURCES = src/printme.c src/alloc_rate.cc ${JEMALLOC}
alloc_rate_mc_jemalloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=MOODY_CAMEL_QUEUE ${AM_CPPFLAGS} ${JEMALLOCFLAGS}

bin_PROGRAMS += alloc_rate_folly_malloc
alloc_rate_folly_malloc_SOURCES = src/printme.c src/alloc_rate.cc folly/folly/detail/Futex.cpp
alloc_rate_folly_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=FOLLY_QUEUE -I$(top_srcdir)/folly ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_mc_malloc
alloc_rate_mc_malloc_SOURCES = src/printme.c src/alloc_rate.cc
alloc_rate_mc_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=MOODY_CAMEL_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_cloudius_malloc
alloc_rate_cloudius_malloc_SOURCES = src/printme.c src/alloc_rate.cc
alloc_rate_cloudius_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=CLOUDIUS_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_natsys_malloc
alloc_rate_natsys_malloc_SOURCES = src/printme.c src/alloc_rate.cc
alloc_rate_natsys_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=NATSYS_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_vyukov_malloc
alloc_rate_vyukov_malloc_SOURCES = src/printme.c src/alloc_rate.cc
alloc_rate_vyukov_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=VYUKOV_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_tbb_malloc
alloc_rate_tbb_malloc_SOURCES = src/printme.c src/alloc_rate.cc \
                    concurrentqueue/benchmarks/tbb/tbb_misc.cpp \
                    concurrentqueue/benchmarks/tbb/cache_aligned_allocator.cpp \
                    concurrentqueue/benchmarks/tbb/dynamic_link.cpp
alloc_rate_tbb_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=TBB_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_boost_malloc
alloc_rate_boost_malloc_SOURCES = src/printme.c src/alloc_rate.cc
alloc_rate_boost_malloc_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=BOOST_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_folly_li
alloc_rate_folly_li_SOURCES = src/printme.c src/alloc_rate.cc folly/folly/detail/Futex.cpp lockless_allocator/ll_alloc.c
alloc_rate_folly_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=FOLLY_QUEUE -I$(top_srcdir)/folly ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_mc_li
alloc_rate_mc_li_SOURCES = src/printme.c src/alloc_rate.cc lockless_allocator/ll_alloc.c
alloc_rate_mc_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=MOODY_CAMEL_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_cloudius_li
alloc_rate_cloudius_li_SOURCES = src/printme.c src/alloc_rate.cc lockless_allocator/ll_alloc.c
alloc_rate_cloudius_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=CLOUDIUS_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_natsys_li
alloc_rate_natsys_li_SOURCES = src/printme.c src/alloc_rate.cc lockless_allocator/ll_alloc.c
alloc_rate_natsys_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=NATSYS_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_vyukov_li
alloc_rate_vyukov_li_SOURCES = src/printme.c src/alloc_rate.cc lockless_allocator/ll_alloc.c
alloc_rate_vyukov_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=VYUKOV_QUEUE ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_tbb_li
alloc_rate_tbb_li_SOURCES = src/printme.c src/alloc_rate.cc \
                    concurrentqueue/benchmarks/tbb/tbb_misc.cpp \
                    concurrentqueue/benchmarks/tbb/cache_aligned_allocator.cpp \
                    concurrentqueue/benchmarks/tbb/dynamic_link.cpp \
                    lockless_allocator/ll_alloc.c
alloc_rate_tbb_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=TBB_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}

bin_PROGRAMS += alloc_rate_boost_li
alloc_rate_boost_li_SOURCES = src/printme.c src/alloc_rate.cc lockless_allocator/ll_alloc.c
alloc_rate_boost_li_CPPFLAGS = -DALLOC_METHOD=MALLOC_ALLOC -DQUEUE_METHOD=BOOST_QUEUE -I$(top_srcdir)/concurrentqueue/benchmarks ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_pthread
lockrate_pthread_SOURCES = src/printme.c src/lockrate.c
lockrate_pthread_CPPFLAGS = -DLOCK_METHOD=PTHREAD ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_pthread_spinlock
lockrate_pthread_spinlock_SOURCES = src/printme.c src/lockrate.c
lockrate_pthread_spinlock_CPPFLAGS = -DLOCK_METHOD=PTHREAD_SPINLOCK ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_clh
lockrate_clh_SOURCES = src/printme.c src/lockrate.c
lockrate_clh_CPPFLAGS = -DLOCK_METHOD=CLH_LOCK ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_mpsc
lockrate_mpsc_SOURCES = src/printme.c src/lockrate.c
lockrate_mpsc_CPPFLAGS = -DLOCK_METHOD=MPSC_LOCK ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_tidex
lockrate_tidex_SOURCES = src/printme.c src/lockrate.c
lockrate_tidex_CPPFLAGS = -DLOCK_METHOD=TIDEX_LOCK ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_ticket
lockrate_ticket_SOURCES = src/printme.c src/lockrate.c
lockrate_ticket_CPPFLAGS = -DLOCK_METHOD=TICKET_LOCK ${AM_CPPFLAGS}

bin_PROGRAMS += lockrate_tidex_nps
lockrate_tidex_nps_SOURCES = src/printme.c src/lockrate.c
lockrate_tidex_nps_CPPFLAGS = -DLOCK_METHOD=TIDEX_NPS_LOCK ${AM_CPPFLAGS}
