#include "bench.h"

void _random_key(char *key,int length) {
	int i;
	char salt[36]= "abcdefghijklmnopqrstuvwxyz0123456789";

	for (i = 0; i < length; i++)
		key[i] = salt[rand() % 36];
}

void _print_header(int count)
{
	double index_size = (double)((double)(KSIZE + 8 + 1) * count) / 1048576.0;
	double data_size = (double)((double)(VSIZE + 4) * count) / 1048576.0;

	printf("Keys:\t\t%d bytes each\n", 
			KSIZE);
	printf("Values: \t%d bytes each\n", 
			VSIZE);
	printf("Entries:\t%d\n", 
			count);
	printf("IndexSize:\t%.1f MB (estimated)\n",
			index_size);
	printf("DataSize:\t%.1f MB (estimated)\n",
			data_size);

	printf(LINE1);
}

void _print_environment()
{
	time_t now = time(NULL);

	printf("Date:\t\t%s", 
			(char*)ctime(&now));

	int num_cpus = 0;
	char cpu_type[256] = {0};
	char cache_size[256] = {0};

	FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo) {
		char line[1024] = {0};
		while (fgets(line, sizeof(line), cpuinfo) != NULL) {
			const char* sep = strchr(line, ':');
			if (sep == NULL || strlen(sep) < 10)
				continue;

			char key[1024] = {0};
			char val[1024] = {0};
			strncpy(key, line, sep-1-line);
			strncpy(val, sep+1, strlen(sep)-1);
			if (strcmp("model name", key) == 0) {
				num_cpus++;
				strcpy(cpu_type, val);
			}
			else if (strcmp("cache size", key) == 0)
				strncpy(cache_size, val + 1, strlen(val) - 1);	
		}

		fclose(cpuinfo);
		printf("CPU:\t\t%d * %s", 
				num_cpus, 
				cpu_type);

		printf("CPUCache:\t%s\n", 
				cache_size);
	}
}

int main(int argc,char** argv)
{
	long int count;

	int number_of_threads = 1; // Default to 1 thread


	srand(time(NULL));
	if (argc < 6) {
		fprintf(stderr,"Usage: db-bench <write | read | readwrite> <count> <number_of_threads> <read_percent(WRITE null IF NOT USING readwrite)> <random(WRITE 1 FOR RANDOM KEYS, 0 FOR SEQUENTIAL)>\n");
		exit(1);
	}

	count = atoi(argv[2]); //number of keys to process
    number_of_threads = atoi(argv[3]); // number of threads for write test
    int r = atoi(argv[5]);	// random key flag
	_print_environment();
    _print_header(count);
	
	if (strcmp(argv[1], "write") == 0) {
		if (strcmp(argv[4], "null") != 0) { //read percentage, not used in write test
			fprintf(stderr,"Usage for write test: db-bench write <count> <number_of_threads> null <random>\n");
			exit(1);
		}
		_write_test(count, r, number_of_threads); // Pass number of threads to the write test
	} else if (strcmp(argv[1], "read") == 0) {
		if (strcmp(argv[4], "null") != 0) { //read percentage, not used in read test
			fprintf(stderr,"Usage for read test: db-bench read <count> <number_of_threads> null <random>\n");
			exit(1);
		}
		_read_test(count, r, number_of_threads); // Pass number of threads to the read test
	} else if (strcmp(argv[1], "readwrite") == 0) {
		int read_percent = atoi(argv[4]); // percentage of read operations for mixed workload
		if (read_percent < 0 || read_percent > 100) { // Validate read percentage input for read/write test
			fprintf(stderr,"Read percentage must be between 0 and 100 for readwrite test\n");
			exit(1);
		}
		_readwrite_test(count, r, number_of_threads, read_percent); // Pass number of threads and read percentage to the read/write test
	} else {
		fprintf(stderr,"Usage: db-bench <write | read | readwrite> <count> <number_of_threads> <read_percent(WRITE null IF NOT USING readwrite)> <random(WRITE 1 FOR RANDOM KEYS, 0 FOR SEQUENTIAL)>\n");
		exit(1);
	}

	return 1;
}
