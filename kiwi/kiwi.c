#include <string.h>
#include "../engine/db.h"
#include "../engine/variant.h"
#include "bench.h"
#include <pthread.h> // for thread methods
#include <stdlib.h> // for malloc and free
#include <sys/time.h> //for bench timing

#define DATAS ("testdb")

static inline long long time_micros() { // Function to get the current time in microseconds for benchmarking
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

typedef struct {
	int thread_id;       // Unique identifier for the thread
    long int start_index; // starting key index
    long int count;       // number of keys to process
    int r;                // random key flag
    DB* db;               // pointer to the database
    int found;            // number of keys found
	int read_percent;		// percentage of read operations for mixed workload
	long long total_read_time_us; // total time taken for read operations in microseconds
    long long total_write_time_us; // total time taken for write operations in microseconds
    int read_count; // number of read operations performed
    int write_count; // number of write operations performed
} ThreadArg;


void* _write(void* arg) {
	ThreadArg* thread_arg = (ThreadArg*)arg; // Cast the argument to ThreadArg structure
	long int start_index = thread_arg->start_index; // Get the starting index for this thread
	long int count = thread_arg->count; // Get the number of keys this thread should process
	int r = thread_arg->r;	// Get the random key flag for this thread
	DB* db = thread_arg->db;	// Get the database pointer for this thread
	thread_arg->total_write_time_us = 0; // Initialize total write time for this thread
    thread_arg->write_count = 0; // Initialize write count for this thread

	char key[KSIZE + 1];	//moved from _write_test since each thread needs its own key buffer 
	char val[VSIZE + 1];	//moved from _write_test since each thread needs its own value buffer
	Variant sk, sv;			//moved from _write_test since each thread needs its own Variant structures

	for (long int i = start_index; i < start_index + count; i++) { // Loop through the assigned key range for this thread
		memset(key, 0, KSIZE + 1);	// Clear the key buffer for the current thread
		memset(val, 0, VSIZE + 1); // Clear the value buffer for the current thread
		memset(&sk, 0, sizeof(Variant)); // Clear the key Variant structure for the current thread
		memset(&sv, 0, sizeof(Variant)); // Clear the value Variant structure for the current thread

		if (r)	// If random key flag is set, generate a random key
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%ld", i);
		snprintf(val, VSIZE, "val-%ld", i);
		sk.length = KSIZE;	// Set the length of the key Variant structure for the current thread
		sk.mem = key;		// Set the memory pointer of the key Variant structure for the current thread
		sv.length = VSIZE;  // Set the length of the value Variant structure for the current thread
		sv.mem = val;		// Set the memory pointer of the value Variant structure for the current thread

		long long write_start = time_micros(); // Start time for the write operation
		db_add(db, &sk, &sv);	// Add the key-value pair to the database using the current thread's key and value
		long long write_end = time_micros(); // End time for the write operation
		thread_arg->total_write_time_us += (write_end - write_start); // Accumulate the total write time for this thread
        thread_arg->write_count++; // Increment the write count for this thread
	}
	return NULL; // Return NULL to indicate the thread has finished its work
}

void* _read(void* arg) {
	ThreadArg* thread_arg = (ThreadArg*)arg;	// Cast the argument to ThreadArg structure
	long int start_index = thread_arg->start_index;	// Get the starting index for this thread
	long int count = thread_arg->count;	// Get the number of keys this thread should process
	int r = thread_arg->r;		// Get the random key flag for this thread
	thread_arg->found = 0; //Get the initial found count for this thread
	int ret;	// Variable to store the return value of db_get
	DB* db = thread_arg->db; // Get the database pointer for this thread
	thread_arg->total_read_time_us = 0; // Initialize total read time for this thread
    thread_arg->read_count = 0; // Initialize read count for this thread

	char key[KSIZE + 1];	//moved from _read_test since each thread needs its own key buffer
	Variant sk, sv;		//moved from _read_test since each thread needs its own Variant structures

	for (long int i = start_index; i < start_index + count; i++) {	// Loop through the assigned key range for this thread
		memset(key, 0, KSIZE + 1);	// Clear the key buffer for the current thread
		memset(&sk, 0, sizeof(Variant)); // Clear the key Variant structure for the current thread
        memset(&sv, 0, sizeof(Variant)); // Clear the key and value Variant structures for the current thread

		if (r)	// If random key flag is set, generate a random key
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%ld", i);

		sk.length = KSIZE;	// Set the length of the key Variant structure for the current thread
		sk.mem = key;	// Set the memory pointer of the key Variant structure for the current thread
		//read doesnt have sv since we are only interested in the key and it will bring the value if the key is found
		long long t_start = time_micros(); // Start time for the read operation
		ret = db_get(db, &sk, &sv);	// Attempt to get the value associated with the key from the database using the current thread's key
		long long t_end = time_micros(); // End time for the read operation
		thread_arg->total_read_time_us += (t_end - t_start); // Accumulate the total read time for this thread
        thread_arg->read_count++; // Increment the read count for this thread
		if (ret) {	// If the key is found in the database, increment the found count for this thread
			thread_arg->found++;
		}
		if (sv.mem) { // Free the memory allocated for the value if it was found
				free(sv.mem);
		}
	}
	return NULL; // Return NULL to indicate the thread has finished its work
}

void* read_write(void* arg){
	ThreadArg* thread_arg = (ThreadArg*)arg; // Cast the argument to ThreadArg structure
	long int start_index = thread_arg->start_index; // Get the starting index for this thread
	long int count = thread_arg->count; // Get the number of keys this thread should process
	int r = thread_arg->r;	// Get the random key flag for this thread
	DB* db = thread_arg->db;	// Get the database pointer for this thread
	thread_arg->found = 0; //Get the initial found count for this thread
	int ret;
	thread_arg->total_read_time_us = 0; // Initialize total read time for this thread
    thread_arg->total_write_time_us = 0; // Initialize total write time for this thread
    thread_arg->read_count = 0; // Initialize read count for this thread
    thread_arg->write_count = 0; // Initialize write count for this thread

	char key[KSIZE + 1];	//moved from _write_test since each thread needs its own key buffer 
	char val[VSIZE + 1];	//moved from _write_test since each thread needs its own value buffer
	Variant sk, sv;			//moved from _write_test since each thread needs its own Variant structures
	unsigned int seed = time(NULL) ^ thread_arg->thread_id;	// Initialize a unique seed for random number generation for this thread

	for (long int i = start_index; i < start_index + count; i++) { // Loop through the assigned key range for this thread
		memset(key, 0, KSIZE + 1);	// Clear the key buffer for the current thread
		memset(val, 0, VSIZE + 1); // Clear the value buffer for the current thread
		memset(&sk, 0, sizeof(Variant)); // Clear the key Variant structure for the current thread
        memset(&sv, 0, sizeof(Variant)); // Clear the value Variant structure for the current thread

		if (r)	// If random key flag is set, generate a random key
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%ld", i);

		
		sk.length = KSIZE;	// Set the length of the key Variant structure for the current thread
		sk.mem = key;		// Set the memory pointer of the key Variant structure for the current thread
		

		if (rand_r(&seed) % 100 < thread_arg->read_percent) { // Perform a read operation based on the specified read percentage
			long long t_start = time_micros(); // Start time for the read operation
			ret = db_get(db, &sk, &sv); // Attempt to get the value associated with the key from the database using the current thread's key
			long long t_end = time_micros(); // End time for the read operation
			thread_arg->total_read_time_us += (t_end - t_start);// Accumulate the total read time for this thread
            thread_arg->read_count++; // Increment the read count for this thread
			if (ret) { // If the key is found in the database, increment the found count for this thread
				thread_arg->found++;
			}
			if (sv.mem) { // Free the memory allocated for the value if it was found
				free(sv.mem);
			}
		} else {
			snprintf(val, VSIZE, "val-%ld", i);
            sv.length = VSIZE;
            sv.mem = val;
			long long t_start = time_micros(); // Start time for the write operation
			db_add(db, &sk, &sv); // Add the key-value pair to the database using the current thread's key and value
			long long t_end = time_micros(); // End time for the write operation
			thread_arg->total_write_time_us += (t_end - t_start); // Accumulate the total write time for this thread
			thread_arg->write_count++; // Increment the write count for this thread
		}
	}
	return NULL; // Return NULL to indicate the thread has finished its work
}

void _write_test(long int count, int r, int num_threads) // Added num_threads parameter
{
	double cost;
	long long start,end;
	DB* db;

	db = db_open(DATAS);

	start = time_micros();

	pthread_t* threads = malloc(num_threads * sizeof(pthread_t));	// Allocate memory for thread identifiers
    ThreadArg* args = malloc(num_threads * sizeof(ThreadArg));		// Allocate memory for thread arguments
	long int count_per_thread = count / num_threads;
	
	for (int i = 0; i < num_threads; i++) { // Create threads for writing
		args[i].start_index = i * count_per_thread;
		if (i == num_threads - 1) {
			args[i].thread_id = i; // Set the thread ID for this thread
			args[i].count = count - (count_per_thread * i); // Handle remaining keys for the last thread
		} else {
			args[i].count = count_per_thread;
		}
		args[i].r = r;
		args[i].db = db;
		pthread_create(&threads[i], NULL, _write, &args[i]);
	}
	long long global_write_us = 0; // Variable to accumulate total write time across all threads
    int global_writes = 0; // Variable to accumulate total write count across all threads

	for (int i = 0; i < num_threads; i++) { // Wait for all threads to finish
		pthread_join(threads[i], NULL);
		global_write_us += args[i].total_write_time_us; // Accumulate total write time from all threads
		global_writes += args[i].write_count; // Accumulate total write count from all threads
	}

	free(threads);  // Free the allocated memory for thread identifiers
	free(args);     // Free the allocated memory for thread arguments

	db_close(db);

	end = time_micros();
	cost = (double)(end - start) / 1000000.0;

	double write_lat; // Variable to store average write latency
	if (global_writes > 0) { // Calculate average write latency if there were any writes performed
		write_lat = (double)global_write_us / global_writes;
	} else {
		write_lat = 0;
	}
	printf("\n================= WRITE TEST =================\n");
    printf("Threads      : %d\n", num_threads);
    printf("Total Ops    : %ld (100%% Writes)\n", count);
    printf("Total Time   : %.3f sec\n", cost);
    printf("Throughput   : %.0f ops/sec\n", (double)count / cost);
    printf("Write Latency: %.2f us/op\n", write_lat);
    printf("==============================================\n");	
}

void _read_test(long int count, int r, int num_threads) // Added num_threads parameter
{
	int total_found = 0; // Initialize total found count
	double cost;
	long long start,end;
	DB* db;

	db = db_open(DATAS);
	start = time_micros();
	pthread_t* threads = malloc(num_threads * sizeof(pthread_t)); // Allocate memory for thread identifiers
	ThreadArg* args = malloc(num_threads * sizeof(ThreadArg)); 	  // Allocate memory for thread arguments
	long int count_per_thread = count / num_threads; // Calculate the number of keys each thread should process

	for (int i = 0; i < num_threads; i++) { // Create threads for reading
		args[i].thread_id = i; // Set the thread ID for this thread
		args[i].start_index = i * count_per_thread; // Set the starting index for each thread
		if (i == num_threads - 1) { 
			args[i].count = count - (count_per_thread * i); // Handle remaining keys for the last thread
		} else {
			args[i].count = count_per_thread;
		}
		args[i].r = r;
		args[i].db = db;
		pthread_create(&threads[i], NULL, _read, &args[i]);
	}

	long long global_read_us = 0;
    int global_reads = 0;

	for (int i = 0; i < num_threads; i++) { // Wait for all threads to finish
		pthread_join(threads[i], NULL);
		total_found += args[i].found; // Accumulate the total found count from all threads
		global_read_us += args[i].total_read_time_us;
		global_reads += args[i].read_count;
	}

	free(threads); // Free the allocated memory for thread identifiers
	free(args);    // Free the allocated memory for thread arguments

	db_close(db);

	end = time_micros();
	cost = (double)(end - start) / 1000000.0;
	double read_lat; // Variable to store average read latency
	if (global_reads > 0) { // Calculate average read latency if there were any reads performed
		read_lat = (double)global_read_us / global_reads;
	} else {
		read_lat = 0;
	}
	printf("\n================= READ TEST ==================\n");
    printf("Threads      : %d\n", num_threads);
    printf("Total Ops    : %ld (100%% Reads, Found: %d)\n", count, total_found);
    printf("Total Time   : %.3f sec\n", cost);
    printf("Throughput   : %.0f ops/sec\n", (double)count / cost);
    printf("Read Latency : %.2f us/op\n", read_lat);
    printf("==============================================\n");
}

void _readwrite_test(long int count, int r, int num_threads, int read_percent) // Added num_threads and read_percent parameters
{
    int total_found = 0; 	// Initialize total found count for read operations
    double cost; 	
    long long start, end;
    DB* db;

    db = db_open(DATAS);
    start = time_micros();

    pthread_t* threads = malloc(num_threads * sizeof(pthread_t)); // Allocate memory for thread identifiers
    ThreadArg* args = malloc(num_threads * sizeof(ThreadArg)); // Allocate memory for thread arguments
    
    long int count_per_thread = count / num_threads; // Calculate the number of keys each thread should process

    for (int i = 0; i < num_threads; i++) {	// Create threads for mixed read/write operations
        args[i].thread_id = i;
        args[i].start_index = i * count_per_thread;
        if (i == num_threads - 1) {
			args[i].count = count - (count_per_thread * i); // Handle remaining keys for the last thread
		} else {
			args[i].count = count_per_thread;
		}
        args[i].r = r;
        args[i].db = db;
        args[i].read_percent = read_percent; 

        pthread_create(&threads[i], NULL, read_write, &args[i]);
    }

	long long global_read_us = 0; // Variable to accumulate total read time across all threads
    long long global_write_us = 0; // Variable to accumulate total write time across all threads
    int global_reads = 0; // Variable to accumulate total read count across all threads
    int global_writes = 0; // Variable to accumulate total write count across all threads

    for (int i = 0; i < num_threads; i++) { // Wait for all threads to finish and accumulate the total found count
        pthread_join(threads[i], NULL);
        total_found += args[i].found;
		global_read_us += args[i].total_read_time_us; // Accumulate total read time from all threads
		global_reads += args[i].read_count; // Accumulate total read count from all threads
		global_write_us += args[i].total_write_time_us; // Accumulate total write time from all threads
		global_writes += args[i].write_count; // Accumulate total write count from all
    }

    free(threads); // Free the allocated memory for thread identifiers
    free(args); // Free the allocated memory for thread arguments
    db_close(db);

    end = time_micros();
    cost = (double)(end - start) / 1000000.0;

	double avg_lat = 0, read_lat = 0, write_lat = 0;
    long total_ops = global_reads + global_writes;
	if (global_reads > 0) {
		read_lat = (double)global_read_us / global_reads;
	}else {
		read_lat = 0;
	}
	if (global_writes > 0) {
		write_lat = (double)global_write_us / global_writes;
	} else {
		write_lat = 0;
	}
	if (total_ops > 0) {
		avg_lat = (double)(global_read_us + global_write_us) / total_ops;
	} else {
		avg_lat = 0;
	}

	printf("\n============== READ/WRITE TEST ===============\n");
    printf("Threads      : %d\n", num_threads);
    printf("R/W Ratio    : %d%% Reads / %d%% Writes\n", read_percent, 100 - read_percent);
    printf("Total Ops    : %ld (Found: %d)\n", count, total_found);
    printf("Total Time   : %.3f sec\n", cost);
    printf("Throughput   : %.0f ops/sec\n", (double)total_ops / cost);
    printf("Avg Latency  : %.2f us/op\n", avg_lat);
    printf("Read Latency : %.2f us/op\n", read_lat);
    printf("Write Latency: %.2f us/op\n", write_lat);
    printf("==============================================\n");
}
