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

Developing
----------

You can freely change the code in the files. GL-STM is a separate git brach that you can always access. Details about branches in git can be found: <https://www.git-scm.com/book/en/v2/Git-Branching-Basic-Branching-and-Merging>

Compiling
---------

You can compile the project with `make` in the base folder. As a result, this invocation creates:

1. `libsstm.a` STM library with the STM system implementation;
2. `bank` executable. A simple STM benchmark that resembles a bank;
3. `ll` executable. A simple STM linked list implementation.

You can use the `./scripts/create_glstm.sh` from the base folder to create the GL-STM versions of bank and ll, as well as your implementations. The GL-STM version executables are named `bank_glstm` and `ll_glstm`.

Executing
---------

You can run the two benchmarks with `./bank` and `./ll`. Both executables support the `-h` flag that prints the parameters they support.

You can use the `./scripts/benchmark.sh` from the base folder to execute the workloads that we will evaluate your solutions on.
