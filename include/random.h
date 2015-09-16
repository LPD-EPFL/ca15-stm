#ifndef _H_RANDOM_
#define _H_RANDOM_

#include <malloc.h>

extern __thread unsigned long* seeds; 

#if defined(__i386__)
static inline uint64_t
getticks(void) 
{
  ticks ret;

  __asm__ __volatile__("rdtsc" : "=A" (ret));
  return ret;
}
#elif defined(__x86_64__)
static inline uint64_t
 getticks(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}
#endif

static inline unsigned long* 
seed_rand() 
{
  seeds = (unsigned long*) memalign(64, 64);
  seeds[0] = getticks() % 123456789;
  seeds[1] = getticks() % 362436069;
  seeds[2] = getticks() % 521288629;
  return seeds;
}

static inline void
free_rand()
{
  free(seeds);
}

//Marsaglia's xorshf generator
static inline unsigned long
xorshf96(unsigned long* x, unsigned long* y, unsigned long* z)  //period 2^96-1
{
  unsigned long t;
  (*x) ^= (*x) << 16;
  (*x) ^= (*x) >> 5;
  (*x) ^= (*x) << 1;

  t = *x;
  (*x) = *y;
  (*y) = *z;
  (*z) = t ^ (*x) ^ (*y);

  return *z;
}

static inline unsigned long
fast_rand()
{
  return xorshf96(seeds, seeds + 1, seeds + 2);
}

#endif
