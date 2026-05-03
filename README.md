# Key-Value Store Synchronization

## Overview
This repository contains the implementation of a multi-threaded synchronization mechanism for a Key-Value store database, developed as part of an Operating Systems university assignment. The core objective is to ensure data integrity during concurrent read and write operations by employing a **Writer-Priority** synchronization mechanism.

## Features
* **Thread-Safe Operations:** Uses POSIX threads (pthreads) to handle concurrent database accesses by initializing mutex values and condition signals.
* **Writer Priority:** Implements writer priority to prevent writer starvation, ensuring that continuously arriving readers do not permanently block waiting writers from updating the database.
* **Concurrent Reads:** Allows multiple readers to access the database simultaneously when no writers are active or waiting, significantly boosting execution speed for read operations.
* **Exclusive Writes:** Ensures only a single writer can modify the database at any given time (e.g., adding to the memtable), preventing race conditions where multiple writers could overwrite each other without the first being read.
* **Deadlock Resolution:** Fixes a critical deadlock in the sst.c file between the sst->lock and sst->cv_lock during the merge and compaction processes by temporarily releasing and re-acquiring the lock in the merge_thread.
* **Benchmarking Tools:** Extended the bench.c and kiwi.c test suites to support multi-threading execution. It allows passing the number of threads dynamically from the command line and testing mixed workloads with specific read/write percentages.

## Architecture Details
* db.h & db.c: Introduced active_readers, active_writers, waiting_readers, and waiting_writers counters within the DB struct. Also added a pthread_mutex_t lock and condition variables safeToRead / safeToWrite to protect critical sections during db_add and db_get executions.
* kiwi.c: Refactored _write_test and _read_test into threaded versions, creating thread structures (ThreadArg) to dynamically divide the key generation and processing workload among multiple running threads.
* sst.c: Solved deadlocks related to thread scheduling during SST file compactions down the tree levels.

## Evaluation & Performance
* **Read-Heavy Workloads:** Throughput increases significantly as the number of threads increases, because read operations happen in parallel without writers blocking them.
* **Write-Heavy Workloads:** Show increased latency and lower throughput due to exclusive writer locking and the overhead of mandatory memory compactions to the hard drive. However, this trade-off is completely necessary to guarantee data integrity and avoid starvation.
* **Mixed (Read-Write) Workloads:** As the percentage of write operations increases compared to reads, overall throughput drops and latency increases. This occurs for two reasons: firstly, more frequent compactions temporarily block both readers and writers, and secondly, the writer-priority mechanism forces readers to wait. This prioritization is directly reflected in the increased read latency, as waiting writers are always allowed to enter the critical section before the waiting readers. Overall, the implementation is highly efficient for read-heavy workloads, and while speed is sacrificed in balanced or write-heavy scenarios, it successfully prevents starvation.

## Author
* **Ioannis Drivas** 
