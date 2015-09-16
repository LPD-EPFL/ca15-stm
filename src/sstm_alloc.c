#include "sstm_alloc.h"

__thread sstm_alloc_t sstm_allocator = { .n_allocs = 0 };
__thread sstm_alloc_t sstm_freeing = { .n_allocs = 0 };

void*
sstm_tx_alloc(size_t size)
{
  assert(sstm_allocator.n_allocs < SSTM_ALLOC_MAX_ALLOCS);
  void* m = malloc(size);

  /* 
     you need to keep track for allocations, so that if a TX
     aborts, you free that memory
   */

  return m;
}

void
sstm_tx_free(void* mem)
{
  assert(sstm_freeing.n_allocs < SSTM_ALLOC_MAX_ALLOCS);

  /* 
     you cannot immediately free(mem) as below if you might 
     have TX aborts. You need to keep track of mem frees
     and only make the actual free happen if the TX is commited
  */

  free(mem);
}

void
sstm_alloc_on_abort()
{

}

void
sstm_alloc_on_commit()
{

}
