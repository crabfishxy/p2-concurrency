/*
 * common.h
 *
 *  Created on: Jun 25, 2020
 *      Author: xzl
 */

#ifndef LAB2B_COMMON_H_
#define LAB2B_COMMON_H_

#include <pthread.h>

//typedef unsigned __int128  my_key_t; // diffcult to printf
typedef unsigned long  my_key_t;


my_key_t getRandomKey();

struct prog_config {
	int iterations;
	int numThreads;
	int mutexFlag;
//	int spinLockFlag;
	int numParts; // task granularity
};

typedef struct prog_config prog_config;

void getTestName(struct prog_config *config, char *buf, int buflen);

void print_csv_line(char* test, int threadNum, int iterations,
		int numList, int numOperation, long long runTime);

struct prog_config parse_config(int argc, char **argv);

void alloc_locks(pthread_mutex_t **mutexes, int n_mutex,
		int **spinlocks, int n_spinlocks);

void free_locks(pthread_mutex_t *mutexes, int nmutex, int *spinlocks);


#endif /* LAB2B_COMMON_H_ */