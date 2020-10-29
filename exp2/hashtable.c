#include <stdio.h>
#include <stdlib.h>

#include <time.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>

#ifdef USE_VTUNE
#include <ittnotify.h>
#endif

#include "common.h"  // xzl
#include "measure.h"  // xzl

//SortedList_t* lists;
GHashTable* tables[1000];
// #ifdef USE_PREALLOC
// SortedListElement_t* elements;
// #else
// my_key_t *keys;
// #endif

int* spinLocks = NULL;
pthread_mutex_t* mutexes = NULL;

int the_n_elements = 0;

int table_nums = 1;

struct prog_config the_config;

long long runTime = 0;

#define ONE_BILLION 1000000000L;

#ifdef USE_VTUNE
	__itt_domain * itt_domain = NULL;
	__itt_string_handle * sh_sort = NULL; // the overall task name
	__itt_string_handle ** sh_parts = NULL; // per part task name

	#define vtune_task_begin(X) __itt_task_begin(itt_domain, __itt_null, __itt_null, sh_parts[X])
	#define vtune_task_end() __itt_task_end(itt_domain)
#else
	#define vtune_task_begin(X)
	#define vtune_task_end()
#endif

void print_errors(char* error){
    if(strcmp(error, "clock_gettime") == 0){
        fprintf(stderr, "Error initializing clock time\n");
        exit(1);
    }
    if(strcmp(error, "thread_create") == 0){
        fprintf(stderr, "Error creating threads.\n");
        exit(2);
    }
    if(strcmp(error, "thread_join") == 0){
        fprintf(stderr, "Error with pthread_join.\n");
        exit(2);
    }
    if(strcmp(error, "mutex") == 0){
        fprintf(stderr, "Error with pthread_join. \n");
        exit(2);
    }
    if(strcmp(error, "segfault") == 0){
        fprintf(stderr, "Segmentation fault caught! \n");
        exit(2);
    }
    if(strcmp(error, "size") == 0){
        fprintf(stderr, "Sorted List length is not zero. List Corrupted\n");
        exit(2);
    }
    if(strcmp(error, "lookup") == 0){
        fprintf(stderr, "Could not retrieve inserted element due to corrupted list.\n");
        exit(2);
    }
    if(strcmp(error, "length") == 0){
        fprintf(stderr, "Could not retrieve length because list is corrupted.\n");
        exit(2);
    }
    if(strcmp(error, "delete") == 0){
        fprintf(stderr, "Could not delete due to corrupted list. \n");
        exit(2);
    }
}

void signal_handler(int sigNum){
    if(sigNum == SIGSEGV){
        print_errors("segfault");
    }
}//signal handler for SIGSEGV


// #ifdef USE_PREALLOC
// // @idx: the idx in the input key table
// SortedListElement_t *get_element(int idx) {
// 	return &elements[idx];
// }

// SortedListElement_t *alloc_elements(int numElements) {
//     fprintf(stderr, "init %d elements\n", numElements);

//     SortedListElement_t * elements = malloc(sizeof(SortedListElement_t) * numElements);
//     assert(elements);

// #ifdef USE_INIT_ELEMENTS
//     for(int i = 0; i < numElements; i++){
//         elements[i].key = getRandomKey();
//         // debugging
// //        if (i < 10)
// //        	printf("random keys: %lx \n", elements[i].key);
//     }
// #endif

//     return elements;
// }

// #else

// my_key_t *alloc_keys(int numElements){
//     fprintf(stderr, "init %d elements", numElements);

//     my_key_t * keys = malloc(sizeof(my_key_t) * numElements);
//     assert(keys);

// #ifdef USE_INIT_ELEMENTS
//     for(int i = 0; i < numElements; i++)
//     	keys[i] = getRandomKey();
// #endif

//     return keys;
// }

// SortedListElement_t *get_element(int idx) {
// 	SortedListElement_t *p = malloc(sizeof(SortedListElement_t));
// 	assert(p);
// 	p->key = keys[idx];
// 	return p;
// }
// #endif

int simple_hash(int key){
    return key % table_nums;
}

void* thread_func(void* thread_id){
    int id = *((int*)thread_id);

    pthread_mutex_lock(&mutexes[0]);
    k2_measure("tr start");
    pthread_mutex_unlock(&mutexes[0]);

//    printf("i'm thread %d\n", id);

    int per_part = the_n_elements / the_config.numThreads;

#ifdef USE_MULTITABLES
    //	#ifdef USE_VTUNE
    //			__itt_task_begin(itt_domain, __itt_null, __itt_null, sh_parts[id]);
    //	#endif
        vtune_task_begin(id);
    		for (int i = per_part * id; i < per_part * (id + 1); i++) {
                int table_index = simple_hash(i);
                pthread_mutex_lock(&mutexes[table_index]);
			    g_hash_table_insert (tables[table_index], GINT_TO_POINTER (i), GINT_TO_POINTER (i));
			    pthread_mutex_unlock(&mutexes[table_index]);
    		}
    //	#ifdef USE_VTUNE
    //			__itt_task_end(itt_domain);
    //	#endif // USE_VTUNE
    		vtune_task_end();
#else // use biglock
	for (int i = per_part * id; i < per_part * (id + 1); i++) {
			// we carefully do malloc() w/o grabbing lock
//			SortedListElement_t *p = malloc(sizeof(SortedListElement_t));
//			assert(p);
//			p->key = keys[i];

			pthread_mutex_lock(&mutexes[0]);
			g_hash_table_insert (tables[0], GINT_TO_POINTER (i), GINT_TO_POINTER (i));
			pthread_mutex_unlock(&mutexes[0]);
	 }
#endif // USB_LB

//    int ll = SortedList_length(&lists[id]);
//    fprintf(stderr, "tr -- list %d: %d items per_part %d\n", id, ll, per_part);

    pthread_mutex_lock(&mutexes[0]);
    k2_measure("tr done");
    pthread_mutex_unlock(&mutexes[0]);

    return NULL;
}

int main(int argc, char** argv) {

    the_config = parse_config(argc, argv);

		int numThreads = the_config.numThreads;
		int iterations = the_config.iterations;
		int numParts = the_config.numParts;
        table_nums = numParts;

    signal(SIGSEGV, signal_handler);

    k2_measure("init");

    srand((unsigned int) time(NULL));

    the_n_elements = numThreads * iterations;
    printf("the_n_elements: %d\n", the_n_elements);
#ifdef USE_MULTITABLES
    alloc_locks(&mutexes, numParts, &spinLocks, 0);
#else
    alloc_locks(&mutexes, 1, NULL, 0);  // multilists or biglock: only 1 mutex, no spinlocks
#endif

#if defined(USE_MULTITABLES)
    for(int i = 0; i < numParts; i ++){
        tables[i] = g_hash_table_new (NULL, NULL);
    }
#else
    tables[0] = g_hash_table_new (NULL, NULL);
#endif
    
#ifdef USE_VTUNE
    itt_domain = __itt_domain_create("my domain");
		__itt_thread_set_name("my main");

		// pre create here, instead of doing it inside tasks
		sh_parts = malloc(sizeof(__itt_string_handle *) * numThreads);
		assert(sh_parts);
		char itt_task_name[32];
		for (int i = 0; i < numThreads; i++) {
			snprintf(itt_task_name, 32, "part-%d", i);
			sh_parts[i] = __itt_string_handle_create(itt_task_name);
		}
#endif

    k2_measure("init done");

    pthread_t threads[numThreads];
    int thread_id[numThreads];

    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0){
        print_errors("clock_gettime");
    }
    
#ifdef USE_VTUNE
    __itt_task_begin(itt_domain, __itt_null, __itt_null,
    					__itt_string_handle_create("list"));
#endif

    for(int i = 0; i < numThreads; i++){
        thread_id[i] = i;
        int rc = pthread_create(&threads[i], NULL, thread_func, &thread_id[i]);
        if(rc < 0){
            print_errors("thread_create");
        }
    }

    k2_measure("tr launched");

    for(int i = 0; i < numThreads; i++){
        int rc = pthread_join(threads[i], NULL);
        if(rc < 0){
            print_errors("thread_join");
        }
    }
    
#ifdef USE_VTUNE
    __itt_task_end(itt_domain);
    free(sh_parts);
#endif

    k2_measure("tr joined");

    if(clock_gettime(CLOCK_MONOTONIC, &end) < 0){
        print_errors("clock_gettime");
    }
    
    long long diff = (end.tv_sec - start.tv_sec) * ONE_BILLION;
    diff += end.tv_nsec;
    diff -= start.tv_nsec;
    
    // we're done. correctness check up
    {
    	long total = 0;
#if defined(USE_MULTITABLES)
			for(int i = 0; i < the_n_elements; i++) {
                int table_index = simple_hash(i);
                int val = GPOINTER_TO_INT (g_hash_table_lookup (tables[table_index], GINT_TO_POINTER (i)));
                assert(val == i);
#else
			for(int i = 0; i < the_n_elements; i++) {
                int val = GPOINTER_TO_INT (g_hash_table_lookup (tables[0], GINT_TO_POINTER (i)));
                assert(val == i);
#endif
                
			}
			fprintf(stderr, "\ntotal %ld items\n", total);
    }

    k2_measure_flush();

    int numOpts = iterations * numThreads; //get number of operations

    char testname[32];
    getTestName(&the_config, testname, 32);
    print_csv_line(testname, numThreads, iterations, numParts, numOpts, diff);

    // --- clean up ---- //
    free_locks(mutexes, 1, spinLocks);

    exit(0);
}



