#include "sstm.h"

LOCK_LOCAL_DATA;
__thread sstm_metadata_t sstm_meta;	 /* per-thread metadata */
sstm_metadata_global_t sstm_meta_global; /* global metadata */

/* initializes the TM runtime 
   (e.g., allocates the locks that the system uses ) 
*/
void
sstm_start()
{
  INIT_LOCK(&sstm_meta_global.glock);
}

/* terminates the TM runtime
   (e.g., deallocates the locks that the system uses ) 
*/
void
sstm_stop()
{
}

/* prints the TM system stats
****** DO NOT TOUCH *********
*/
void
sstm_print_stats(double dur_s)
{
  printf("# Commits: %-10zu - %.0f /s\n",
	 sstm_meta_global.n_commits,
	 sstm_meta_global.n_commits / dur_s);
  printf("# Aborts : %-10zu - %.0f /s\n",
	 sstm_meta_global.n_aborts,
	 sstm_meta_global.n_aborts / dur_s);
}

/* initializes thread local data
   (e.g., allocate a thread local counter)
 */
void
sstm_thread_start()
{
}

/* terminates thread local data
   (e.g., deallocate a thread local counter)
****** DO NOT CHANGE THE EXISTING CODE*********   
 */
void
sstm_thread_stop()
{
  __sync_fetch_and_add(&sstm_meta_global.n_commits, sstm_meta.n_commits);
  __sync_fetch_and_add(&sstm_meta_global.n_aborts, sstm_meta.n_aborts);
}


/* transactionally reads the value of addr
*/
inline uintptr_t
sstm_tx_load(volatile uintptr_t* addr)
{
  return *addr;
}

/* transactionally writes val in addr
*/
inline void
sstm_tx_store(volatile uintptr_t* addr, uintptr_t val)
{
  *addr = val;
}

/* cleaning up in case of an abort 
   (e.g., flush the read or write logs)
*/
void
sstm_tx_cleanup()
{
  sstm_alloc_on_abort();
  sstm_meta.n_aborts++;
}

/* tries to commit a transaction
   (e.g., validates some version number, and/or
   acquires a couple of locks)
 */
void
sstm_tx_commit()
{
  UNLOCK(&sstm_meta_global.glock);	       
  sstm_alloc_on_commit();
  sstm_meta.n_commits++;		
}
