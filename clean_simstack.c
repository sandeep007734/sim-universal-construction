#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "tvec.h"
#include "pool.h"
#include "thread.h"

#define LOCAL_POOL_SIZE            16

#define POP                        -1

#define DEBUG

#include "init_code.c"
#include "stack_op.c"




static inline RetVal apply_op(SimStackThreadState *th_state, ArgVal arg, int pid) {
    ToggleVector diffs, l_toggles, pops;
    pointer_t new_sp, old_sp;
    ObjectState *lsp_data, *sp_data;
    int i, j, prefix, mybank, push_counter;
    ArgVal tmp_arg;

	// Getting the bank. For the purpise of our code. Out entire thread will fit in bank  which will be of 32 or 64 depending on the architecture.
    mybank = TVEC_GET_BANK_OF_BIT(pid);				// This is always returning 0. ANd by seeing the code for GET_BANK it will given something other than 0 only for pid strictly less than 4.
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);			// This is reversing the bit at mybank?
    TVEC_NEGATIVE_BANK(&th_state->toggle, &th_state->toggle, mybank);
    lsp_data = (ObjectState *)&pool[pid * LOCAL_POOL_SIZE + th_state->local_index];
    announce[pid] = arg;                                                  // announce the operation
    TVEC_ATOMIC_ADD_BANK(&a_toggles, &th_state->toggle, mybank);                    // toggle pid's bit in a_toggles, Fetch&Add acts as a full write-barrier
#if N_THREADS > USE_CPUS
    if (simRandomRange(1, N_THREADS) > 4)
        sched_yield();
#else
    volatile int k;
    int backoff_limit;

    if (simRandomRange(1, N_THREADS) > 1) {
        backoff_limit =  simRandomRange(th_state->backoff >> 1, th_state->backoff);
        for (k = 0; k < backoff_limit; k++)
            ;
    }
#endif

    // This is the loop mentioned in the paper.
    for (j = 0; j < 2; j++) {
        old_sp = sp;		// read reference to struct ObjectState
        sp_data = (ObjectState *)&pool[old_sp.struct_data.index];    // read reference of struct ObjectState in a local variable lsp_data
        TVEC_ATOMIC_COPY_BANKS(&diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(&diffs, &diffs, &th_state->my_bit, mybank);           // determine the set of active processes
        if (TVEC_IS_SET(diffs, pid))                                      // if the operation has already been applied return
            break;
        *lsp_data = *sp_data;
        l_toggles = a_toggles;                                            // This is an atomic read, since a_toogles is volatile
        if (old_sp.raw_data != sp.raw_data)
            continue;
        diffs = TVEC_XOR(lsp_data->applied, l_toggles);
        push_counter = 0;
        TVEC_SET_ZERO(&pops);
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            ReadPrefetch(&announce[prefix]);
            ReadPrefetch(&announce[prefix + 8]);
            ReadPrefetch(&announce[prefix + 16]);
            ReadPrefetch(&announce[prefix + 24]);
            while (diffs.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);
                proc_id = prefix + pos;
                diffs.cell[i] ^= 1L << pos;
                tmp_arg = announce[proc_id];
                if (tmp_arg == POP) {
                    pops.cell[i] |= 1L << pos;
                } else {
                    push(th_state, lsp_data, tmp_arg);
                    push_counter++;
                }
            }
        }
		
        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (pops.cell[i] != 0L) {
                register int pos, proc_id;

                pos = bitSearchFirst(pops.cell[i]);
                proc_id = prefix + pos;
                pops.cell[i] ^= 1L << pos;
                pop(lsp_data, proc_id);
            }
        }

        lsp_data->applied = l_toggles;                                       // change applied to be equal to what was read in a_toggles
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;                 // increase timestamp
        new_sp.struct_data.index = LOCAL_POOL_SIZE * pid + th_state->local_index;      // store in mod_dw.index the index in pool where lsp_data will be stored
        if (old_sp.raw_data==sp.raw_data &&
            CAS64(&sp.raw_data, old_sp.raw_data, new_sp.raw_data)) {
            th_state->local_index = (th_state->local_index + 1) % LOCAL_POOL_SIZE;
            th_state->backoff = (th_state->backoff >> 1) | 1;
            return lsp_data->ret[pid];
        }
        else {
            if (th_state->backoff < MAX_BACK) th_state->backoff <<= 1;
            rollback(&th_state->pool_node, push_counter);
        }
    }
    return pool[sp.struct_data.index].ret[pid];         // return the value found in the record stored there
}

pthread_barrier_t barr CACHE_ALIGN;
int64_t d1 CACHE_ALIGN, d2;


static inline void Execute(void* Arg) {
    #ifdef DEBUG
        printf("%s\n", "Starting the Execute");
    #endif
    SimStackThreadState th_state;
    long i;
    long rnum;
    volatile long j;
    int id = (long) Arg;

    #ifdef DEBUG
        printf("%s\n","Initializing the Shared Objects and other variables");
    #endif
    setThreadId(id);
    _thread_pin(id);
    simSRandom(id + 1);
    simStackThreadStateInit(&th_state, id);
    SHARED_OBJECT_INIT(id);
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();
    // Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }
    start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        apply_op(&th_state, (ArgVal) id + 1, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
        apply_op(&th_state, (ArgVal) POP, id);
        rnum = simRandomRange(1, MAX_WORK);
        for (j = 0; j < rnum; j++)
            ;
    }
    stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
}
// This creates a thread and returns it ?
inline static pthread_t StartThread(int arg) {
    #ifdef DEBUG
        printf("Creating a thead with an id of:  %d\n", arg);
    #endif
    long id = (long) arg;
    void *Arg = (void*) id;
    pthread_t thread_p;
    int thread_id;

    pthread_attr_t my_attr;
    pthread_attr_init(&my_attr);
    thread_id = pthread_create(&thread_p, &my_attr, EntryPoint, Arg);

    return thread_p;
}

// This is just creating the thread, setting up the barrier and starting the therad.
// After te thread is done working it does some clean up operations also.
int main(int argc, char *argv[]) {
    pthread_t threads[N_THREADS];
    int i;

    #ifdef DEBUG
        printf("%s\n", "Starting the program...");
    #endif

    init_cpu_counters();
    if (argc != 2) {
        fprintf(stderr, "ERROR: Please set an upper bound for the backoff!. Argc count is : %d it should be 2.\n",argc);
        exit(EXIT_SUCCESS);
    } else {
        sscanf(argv[1], "%d", &MAX_BACK);
    }

    #ifdef DEBUG
        printf("%s\n", "Creating the barrier");
    #endif
    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }
    #ifdef DEBUG
        printf("%s\n", "Creating the Barrier is successfull");
    #endif

    #ifdef DEBUG
        printf("%s\n", "Creating the threads");
    #endif
    // This is actually creating the thread.
    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

    #ifdef DEBUG
        printf("%s\n","Joining the threads");
    #endif
    // Waiting of all the theads to finish there operation.
    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);
    
    d2 = getTimeMillis();

    printf("time: %d\t", (int) (d2 - d1));
    printStats();

#ifdef DEBUG
    fprintf(stderr, "Object state debug counter: %lld\n", (long long int)pool[((pointer_t*)&sp)->struct_data.index].counter);
#endif

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
