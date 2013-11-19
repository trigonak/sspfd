/*   
 *   File: sspfd.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: sspfd interface, structures, and helper functions
 *   sspfd.h is part of ccbench
 *
 * The MIT License (MIT)
 *
 * Copyright (C) 2013  Vasileios Trigonakis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _SSPFD_H_
#define _SSPFD_H_

#include <stdio.h>
#include <inttypes.h>
#include <float.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

/* #include "common.h" */

extern size_t SSPFD_ID;
#define SSPFD_PRINT(args...) printf("[%02lu] ", SSPFD_ID); printf(args); printf("\n"); fflush(stdout)

typedef uint64_t ticks;

#if defined(__i386__)
static inline ticks 
getticks(void) 
{
  ticks ret;

  __asm__ __volatile__("rdtsc" : "=A" (ret));
  return ret;
}
#elif defined(__x86_64__)
static inline ticks
getticks(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#elif defined(__sparc__)
static inline ticks
getticks()
{
  ticks ret;
  __asm__ __volatile__ ("rd %%tick, %0" : "=r" (ret) : "0" (ret)); 
  return ret;
}
#elif defined(__tile__)
#include <arch/cycle.h>
static inline ticks getticks()
{
  return get_cycle_count();
}
#endif


#define DO_TIMINGS

#if !defined(PREFETCHW)
#  if defined(__x86_64__) | defined(__i386__)
#    define PREFETCHW(x) asm volatile("prefetchw %0" :: "m" (*(unsigned long *)x)) /* write */
#  elif defined(__sparc__)
#    define PREFETCHW(x) __builtin_prefetch((const void*) x, 1, 3)
#  elif defined(__tile__)
#    define PREFETCHW(x) tmc_mem_prefetch (x, 64)
#  else
#    warning "You need to define PREFETCHW(x) for your architecture"
#  endif
#endif

typedef struct sspfd_stats
{
  uint64_t num_vals;
  double avg;
  double avg_10p;
  double avg_25p;
  double avg_50p;
  double avg_75p;
  double avg_rst;
  double abs_dev_10p;
  double abs_dev_25p;
  double abs_dev_50p;
  double abs_dev_75p;
  double abs_dev_rst;
  double abs_dev;
  double abs_dev_perc;
  double std_dev_10p;
  double std_dev_25p;
  double std_dev_50p;
  double std_dev_75p;
  double std_dev_rst;
  double std_dev;
  double std_dev_perc;
  double min_val;
  uint64_t min_val_idx;
  double max_val;
  uint64_t max_val_idx;
  uint32_t num_dev_10p;
  uint32_t num_dev_25p;
  uint32_t num_dev_50p;
  uint32_t num_dev_75p;
  uint32_t num_dev_rst;
} sspfd_stats_t;


#define SSPFD_NUM_STORES 2
#define SSPFD_PRINT_MAX 200

extern volatile ticks** sspfd_store;
extern volatile ticks* _sspfd_s;
extern volatile ticks sspfd_correction;
#if !defined(DO_TIMINGS)
#  define SSPFDINIT(num_entries) 
#  define SSPFDI(store) 
#  define SSPFDO(store, entry) 
#  define SSPFDP(store, num_vals) 
#  define SSPFDPN(store, num_vals, num_print)
#else  /* DO_TIMINGS */
#  define SSPFDINIT(num_entries) sspfd_store_init(num_entries)

#  define SSPFDI(store)				\
  {						\
  asm volatile ("");				\
  _sspfd_s[store] = getticks();


#  define SSPFDO(store, entry)						\
  asm volatile ("");							\
  sspfd_store[store][entry] =  getticks() - _sspfd_s[store] - sspfd_correction; \
  }

#  define SSPFDPN(store, num_vals, num_print)				\
  {									\
    uint32_t _i;							\
    uint32_t p = num_print;						\
    if (p > num_vals) { p = num_vals; }					\
    for (_i = 0; _i < p; _i++)						\
      {									\
	printf("[%3d: %4ld] ", _i, (long int) sspfd_store[store][_i]);	\
      }									\
    sspfd_stats_t ad;							\
    sspfd_get_stats(store, num_vals, &ad);				\
    sspfd_print_stats(&ad);						\
  }
#endif /* !DO_TIMINGS */

# define SSPFDPREFTCH(store, entry)		\
  SSPFDI(store);					\
  SSPFDO(store, entry);



inline void sspfd_set_id(size_t id);
inline size_t sspfd_get_id();

void sspfd_store_init(size_t num_stores, size_t num_entries, size_t id);
void sspfd_store_term();
void sspfd_get_stats(const size_t store, const size_t num_vals, sspfd_stats_t* sspfd_stats);
void sspfd_print_stats(const sspfd_stats_t* sspfd_stats);


#endif	/* _SSPFD_H_ */
