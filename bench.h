#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define KSIZE (16)
#define VSIZE (1000)

#define LINE "+-----------------------------+----------------+------------------------------+-------------------+\n"
#define LINE1 "---------------------------------------------------------------------------------------------------\n"

long long get_ustime_sec(void);
void _random_key(char *key,int length);
void _write_test(long int count, int r, int num_threads); // Added num_threads parameter
void _read_test(long int count, int r, int num_threads); // Added num_threads parameter
void _readwrite_test(long int count, int r, int num_threads, int read_percent); //Added num_threads and read_percent parameter
