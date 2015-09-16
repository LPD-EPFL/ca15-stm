#ifndef _SSTM_ALLOC_H_
#define	_SSTM_ALLOC_H_

#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define SSTM_ALLOC_MAX_ALLOCS 16

  typedef struct sstm_alloc
  {
    union
    {
      size_t n_allocs;
      size_t n_frees;
    };
    void* mem[SSTM_ALLOC_MAX_ALLOCS];
  } sstm_alloc_t;

  void*  sstm_tx_alloc(size_t size);
  void sstm_tx_free(void* mem);
  void sstm_alloc_on_abort();
  void sstm_alloc_on_commit();


#ifdef	__cplusplus
}
#endif

#endif	/* _SSTM_ALLOC_H_ */

