/*   
 *   File: lock_if.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: 
 *   lock_if.h is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LOCK_IF_H_
#define _LOCK_IF_H_

#include <stdint.h>

#if !defined(COMPILER_BARRIER)
#  define COMPILER_BARRIER() asm volatile ("" ::: "memory")
#endif

#if !defined(COMPILER_NO_REORDER)
#  define COMPILER_NO_REORDER(exec)		\
  COMPILER_BARRIER();				\
  exec;						\
  COMPILER_BARRIER()
#endif

#if defined(MUTEX)
typedef pthread_mutex_t ptlock_t;
#  define LOCK_LOCAL_DATA                                
#  define PTLOCK_SIZE sizeof(ptlock_t)
#  define INIT_LOCK(lock)				pthread_mutex_init((pthread_mutex_t *) lock, NULL)
#  define DESTROY_LOCK(lock)			        pthread_mutex_destroy((pthread_mutex_t *) lock)
#  define LOCK(lock)					pthread_mutex_lock((pthread_mutex_t *) lock)
#  define TRYLOCK(lock)					pthread_mutex_trylock((pthread_mutex_t *) lock)
#  define UNLOCK(lock)					pthread_mutex_unlock((pthread_mutex_t *) lock)
#elif defined(SPIN)		/* pthread spinlock */
typedef pthread_spinlock_t ptlock_t;
#  define LOCK_LOCAL_DATA                                
#  define PTLOCK_SIZE sizeof(ptlock_t)
#  define INIT_LOCK(lock)				pthread_spin_init((pthread_spinlock_t *) lock, PTHREAD_PROCESS_PRIVATE);
#  define DESTROY_LOCK(lock)			        pthread_spin_destroy((pthread_spinlock_t *) lock)
#  define LOCK(lock)					pthread_spin_lock((pthread_spinlock_t *) lock)
#  define UNLOCK(lock)					pthread_spin_unlock((pthread_spinlock_t *) lock)
#elif defined(TAS)			/* TAS */
typedef volatile size_t ptlock_t;
#  define LOCK_LOCAL_DATA                                
#  define INIT_LOCK(lock)				tas_init(lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)					tas_lock(lock)
#  define TRYLOCK(lock)					tas_trylock(lock)
#  define UNLOCK(lock)					tas_unlock(lock)

#  define TAS_FREE 0
#  define TAS_LCKD 1

static inline void
tas_init(ptlock_t* l)
{
  *l = TAS_FREE;
}

static inline uint32_t
tas_lock(ptlock_t* l)
{
  while (__sync_val_compare_and_swap(l, TAS_FREE, TAS_LCKD) == TAS_LCKD)
    {
      asm volatile("pause");
    }
  return 0;
}

static inline uint32_t
tas_trylock(ptlock_t* l)
{
  return (__sync_val_compare_and_swap(l, TAS_FREE, TAS_LCKD) == TAS_FREE);

}

static inline uint32_t
tas_unlock(ptlock_t* l)
{
  COMPILER_NO_REORDER(*l = TAS_FREE;);
  return 0;
}

#elif defined(TTAS)			/* TTAS */
typedef volatile size_t ptlock_t;
#  define LOCK_LOCAL_DATA                                
#  define INIT_LOCK(lock)				ttas_init(lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)					ttas_lock(lock)
#  define TRYLOCK(lock)					ttas_trylock(lock)
#  define UNLOCK(lock)					ttas_unlock(lock)
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				ttas_init(lock)
#  define GL_DESTROY_LOCK(lock)			
#  define GL_LOCK(lock)					ttas_lock(lock)
#  define GL_TRYLOCK(lock)				ttas_trylock(lock)
#  define GL_UNLOCK(lock)              			ttas_unlock(lock)

#  define TTAS_FREE 0
#  define TTAS_LCKD 1

static inline void
ttas_init(ptlock_t* l)
{
  *l = TTAS_FREE;
}

static inline uint32_t
ttas_lock(ptlock_t* l)
{
  while (1)
    {
      while (*l == TTAS_LCKD)
	{
	  asm volatile("pause");
	}

      if (__sync_val_compare_and_swap(l, TTAS_FREE, TTAS_LCKD) == TTAS_FREE)
	{
	  break;
	}
    }

  return 0;
}

static inline uint32_t
ttas_trylock(ptlock_t* l)
{
  return (__sync_val_compare_and_swap(l, TTAS_FREE, TTAS_LCKD) == TTAS_FREE);
}

static inline uint32_t
ttas_unlock(ptlock_t* l)
{
  COMPILER_NO_REORDER(*l = TTAS_FREE;);
  return 0;
}

#elif defined(TICKET)			/* ticket lock */

struct ticket_st
{
  uint32_t ticket;
  uint32_t curr;
};

typedef struct ticket_st ptlock_t;
#  define LOCK_LOCAL_DATA                                
#  define INIT_LOCK(lock)				ticket_init((volatile ptlock_t*) lock)
#  define DESTROY_LOCK(lock)			
#  define LOCK(lock)					ticket_lock((volatile ptlock_t*) lock)
#  define TRYLOCK(lock)					ticket_trylock((volatile ptlock_t*) lock)
#  define UNLOCK(lock)					ticket_unlock((volatile ptlock_t*) lock)
/* GLOBAL lock */
#  define GL_INIT_LOCK(lock)				ticket_init((volatile ptlock_t*) lock)
#  define GL_DESTROY_LOCK(lock)			
#  define GL_LOCK(lock)					ticket_lock((volatile ptlock_t*) lock)
#  define GL_TRYLOCK(lock)				ticket_trylock((volatile ptlock_t*) lock)
#  define GL_UNLOCK(lock)				ticket_unlock((volatile ptlock_t*) lock)

static inline void
ticket_init(volatile ptlock_t* l)
{
  l->ticket = l->curr = 0;
#  if defined(__tile__)
  MEM_BARRIER;
#  endif
}

#  define TICKET_BASE_WAIT 512
#  define TICKET_MAX_WAIT  4095
#  define TICKET_WAIT_NEXT 128

static inline uint32_t
ticket_lock(volatile ptlock_t* l)
{
  uint32_t ticket = __sync_fetch_and_add(&l->ticket, 1);

  while (ticket != l->curr)
    {
      asm volatile("pause");
    }

  return 0;
}

static inline uint32_t
ticket_trylock(volatile ptlock_t* l)
{
  volatile uint64_t tc = *(volatile uint64_t*) l;
  volatile struct ticket_st* tp = (volatile struct ticket_st*) &tc;

  if (tp->curr == tp->ticket)
    {
      COMPILER_NO_REORDER(uint64_t tc_old = tc;);
      tp->ticket++;
      return CAS_U64((uint64_t*) l, tc_old, tc) == tc_old;
    }
  else
    {
      return 0;
    }
}

static inline uint32_t
ticket_unlock(volatile ptlock_t* l)
{
  COMPILER_NO_REORDER(l->curr++;);
  return 0;
}
#elif defined(MCS)		/* MCS lock */

#  include "mcs.h"

typedef mcs_lock_t ptlock_t;
#define LOCK_LOCAL_DATA                                 __thread mcs_lock_local_t __mcs_local

#  define INIT_LOCK(lock)				mcs_lock_init((mcs_lock_t*) lock)
#  define DESTROY_LOCK(lock)			        mcs_lock_destroy((mcs_lock_t*) lock)
#  define LOCK(lock)					mcs_lock_lock((mcs_lock_t*) lock)
#  define UNLOCK(lock)					mcs_lock_unlock((mcs_lock_t*) lock)     
#endif

#endif	/* _LOCK_IF_H_ */
