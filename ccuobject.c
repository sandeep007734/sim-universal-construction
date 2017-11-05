#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <omp.h>
#include <string.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "ccsynch.h"
#include "thread.h"


volatile Object object CACHE_ALIGN;
CCSynchStruct object_lock CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;


void SHARED_OBJECT_INIT(void) {
    object = 1;
    CCSynchStructInit(&object_lock);   
}

inline static RetVal fetchAndMultiply(void *state, ArgVal arg, int pid) {
    Object *st = (Object *)state;
    (*st) *= arg;
    return *st;
}

// Global Variables
pthread_barrier_t barr;

inline static void Execute(void* Arg) {
    ThreadState lobject_lock;
    long i, rnum;
    volatile int j;
    long id = (long) Arg;

    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
    threadStateInit(&lobject_lock, (int)id);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(-1);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        // perform a fetchAndMultiply operation
        applyOp(&object_lock, &lobject_lock, fetchAndMultiply, (void *)&object, (ArgVal) id, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ; 
    }
    stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return NULL;
}

inline static pthread_t StartThread(int arg) {
    long id = (long) arg;
    void *Arg = (void*) id;
    pthread_t thread_p;
    int thread_id;

    pthread_attr_t my_attr;
    pthread_attr_init(&my_attr);
    thread_id = pthread_create(&thread_p, &my_attr, EntryPoint, Arg);

    return thread_p;
}

int main(void) {
    pthread_t threads[N_THREADS];
    int i;

    init_cpu_counters();
    // Barrier initialization
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

    SHARED_OBJECT_INIT();
    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    d2 = getTimeMillis();

    printf("time: %d\t", (int) (d2 - d1));
    printStats();

#ifdef DEBUG
    fprintf(stderr, "object counter: %d\n", object_lock.counter);
    fprintf(stderr, "rounds: %d\n", object_lock.rounds);
    fprintf(stderr, "Average helping: %f\n", (float)object_lock.counter/object_lock.rounds);
    for (i = 0; i < N_THREADS; i++)
        fprintf(stderr, "thread: %d was %d times combiner (%.1f %%)\n",
                i, object_lock.combiner_counter[i],  100.0*(object_lock.combiner_counter[i]/(float)object_lock.rounds));
    fprintf(stderr, "\n");
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
