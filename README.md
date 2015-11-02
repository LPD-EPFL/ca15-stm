Concurrent Algorithms 2015, STM
===============================

Global-lock STM (GL-STM)
------------------------

We have already implemented the simplest-possible STM algorithm that you will use as a baseline. This algorithms uses a global lock to protect every single transaction. In short, every transaction acquires the lock and then executes the transaction. Notice that because of the global lock, in this algorithm, transactions are never aborted.

More Optimistic STM Algorithms
------------------------------

Your task is to implement a more optimistic STM algorithm. GL-STM does not support any concurrency, even for read-only transactions. More scalable algorithms, such as TL2, NORec, and tinySTM, do allow full concurrency of read-only transactions.

Where to Start
--------------

After reading the papers that we provide in `readers` sub-folder, you must read and understand the code of the current repository (i.e., the GL-STM algorithm).

* `sstm.h`: This file includes all the macros for starting/stopping the STM system, beginning/ending a transaction, reading/writing to memory within a transaction, and allocating/freeing memory within a transaction. Additionally, `sstm.h` includes the `sstm_metadata_t` and `sstm_metadata_global_t` structures, corresponding to per-thread and to global STM metadata, respectively.
* `sstm.c`: This file include the implementations of the STM functions. Look in the code for comments on the purpose of each function. 
* `sstm_alloc.c`: This file include the implementations of the STM functions for memory allocation. Look in the code for comments on the purpose of each function. 
