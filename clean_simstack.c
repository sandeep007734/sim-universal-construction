#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdarg.h>

#define N_THREADS                   100
#define RUNS                        1000

int sync_printf(const char *format, ...);

#include "config.h"
#include "primitives.h"
#include "rand.h"
#include "tvec.h"
#include "pool.h"
#include "thread.h"

#define LOCAL_POOL_SIZE            16

#define POP                        -1
#define PUSH                        1


// #define DEBUG

#include "init_code.c"
#include "stack_op.c"
#include "util_code.c"



static inline RetVal apply_op(SimStackThreadState *th_state, ArgVal arg, int pid) {
    ToggleVector diffs, l_toggles, pops;
    pointer_t new_sp, old_sp;
    ObjectState *lsp_data, *sp_data;
    int i, j, prefix, mybank, push_counter;
    ArgVal tmp_arg;

    mybank = TVEC_GET_BANK_OF_BIT(pid);				
    // if(mybank!=0){
    //     sync_printf("%s\n", "ERROR-----------");
    // }

    // printf("\n%s\n","-----------" );
    // printf("%s %d\n", "my_bit: ", pid);
    // fflush(stdout);                       // Setting i bit/
    // binprintf((&th_state->my_bit)->cell[mybank]);
    // fflush(stdout);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);    
    // printf("%s\n", "rev: my_bit");                       // Setting i bit/
    // binprintf((&th_state->my_bit)->cell[mybank]);

    // printf("%s\n","th_state->toggle" );
    // binprintf((&th_state->toggle)->cell[mybank]);
    TVEC_NEGATIVE_BANK(&th_state->toggle, &th_state->toggle, mybank);   
    // printf("%s\n","beg: th_state->toggle" );
    // binprintf((&th_state->toggle)->cell[mybank]);

    lsp_data = (ObjectState *)&pool[pid * LOCAL_POOL_SIZE + th_state->local_index]; // stack pointer data.
     // printf("%s %p\n","lsp_data", lsp_data );
    // Node *temp = lsp_data->head;
    // if(temp!=NULL)
    //     printf("%s %d\n", "lsp_data: ",temp->index);
    // else
    //     printf("%s\n", "Head was NULL");

    announce[pid] = arg;                                                // Announce the operation.                  

    // printf("%s\n", "a_toggles");
    // binprintf(a_toggles.cell[mybank]);
    TVEC_ATOMIC_ADD_BANK(&a_toggles, &th_state->toggle, mybank);        // Toggle the ith bit at a_toggles.
    // printf("%s\n", "a_toggles");
    // binprintf(a_toggles.cell[mybank]);

    for (j = 0; j < 2; j++) {   
        // printf("%s %d\n","sp index",sp.struct_data.index );  
        // printf("%s %p\n","old_sp", old_sp );        
        old_sp = sp;
        // printf("%s %p\n","sp", sp ); 
        // printf("%s %p\n","old_sp", old_sp );  		                                            // Reference for stack pointer (stack top)
         // printf("%s %p\n","sp", sp );
          // printf("%s %p\n","old_sp", old_sp );
        sp_data = (ObjectState *)&pool[old_sp.struct_data.index];  
         // printf("%s %p\n","sp_data", sp_data );  
        // printf("%s\n", "sp_data applied");
        // binprintf((&sp_data->applied)->cell[mybank]);

        TVEC_ATOMIC_COPY_BANKS(&diffs, &sp_data->applied, mybank);
        TVEC_XOR_BANKS(&diffs, &diffs, &th_state->my_bit, mybank);      // Diffs store the sets of active threads.     

        // printf("%s\n", "diffs");
        // binprintf(diffs.cell[mybank]);
        if (TVEC_IS_SET(diffs, pid)){                                   // If my position is set that means someone is helping me .        
            // printf("%s\n", "Breaking Out");                  
            break;
        }

        // printf("%s %p\n","lsp_data", lsp_data );
        *lsp_data = *sp_data;   
        // printf("%s %p\n","lsp_data", lsp_data );     
        // printf("%s %p\n","l_toggles", l_toggles );                                     // Stack pointer data.
        l_toggles = a_toggles;                                          //
        // printf("%s %p\n","l_toggles", l_toggles );



        if (old_sp.raw_data != sp.raw_data){                            // Index and sequence number should remain saim, else someupdate happened.
            continue;
        }

  
        diffs = TVEC_XOR(lsp_data->applied, l_toggles);                 // Exclude i from the set of operators.
        TVEC_SET_BIT(&diffs, pid);
        TVEC_SET_ZERO(&pops);

        for (i = 0, prefix = 0; i < _TVEC_CELLS_; i++, prefix += _TVEC_BIWORD_SIZE_) {
            while (diffs.cell[i] != 0L) {   // someone is active, or some work is pendig.
                register int pos, proc_id;

                pos = bitSearchFirst(diffs.cell[i]);    // get a thread to help
                proc_id = prefix + pos;                 // finds its thread it

                // printf("%s %d\n", "Work left out for thread", proc_id);
                // binprintf(diffs.cell[mybank]);

                diffs.cell[i] ^= (1L << pos);             // tell everyone that I am doing it
                tmp_arg = announce[proc_id];            // get the work from announce
                if (tmp_arg == POP) {                   // Do the work.
                    // printf("%s\n", "Popping");
                    // pops.cell[i] |= 1L << pos;
                    pop(lsp_data, proc_id);
                } else if(tmp_arg == PUSH) {
                    // printf("%s\n", "Pushing" );
                    push(th_state, lsp_data, tmp_arg);
                    // push_counter++;
                }
            }
        }

// change applied to be equal to what was read in a_toggles
        // printf("%s %p\n","lsp_data->applied", lsp_data->applied ); 
        // printf("%s %p\n","lsp_data", lsp_data ); 
        lsp_data->applied =l_toggles; 
        // printf("%s %p\n","lsp_data", lsp_data ); 
        // printf("%s %p\n","lsp_data->applied", lsp_data->applied );   
        // printf("%s \n", "new applied");
        // binprintf((lsp_data->applied).cell[mybank]);                                    

// increase timestamp
        new_sp.struct_data.seq = old_sp.struct_data.seq + 1;        

// store in mod_dw.index the index in pool where lsp_data will be stored         
        new_sp.struct_data.index = LOCAL_POOL_SIZE * pid + th_state->local_index;  
        // printf("%s %d\n","th_state->index",th_state->local_index );
        // printf("%s %d %d\n", "New seq and idx",new_sp.struct_data.seq, new_sp.struct_data.index);
        // printf("%s %ld %ld \n","OLD raw and SP raw",old_sp.raw_data, sp.raw_data );
        if (old_sp.raw_data==sp.raw_data &&  CAS64(&sp.raw_data, old_sp.raw_data, new_sp.raw_data)) {
            th_state->local_index = (th_state->local_index + 1) % LOCAL_POOL_SIZE;
            return lsp_data->ret[pid];
        }
        // else{
        //     printf("%s\n","CAS Failed." );
        // }
    }
return pool[sp.struct_data.index].ret[pid];         // return the value found in the record stored there
}

pthread_barrier_t barr;
int64_t d1 , d2;

static inline void Execute(void* Arg) {
    SimStackThreadState th_state;
    long i;
    long rnum;
    volatile long j;
    int id = (long) Arg;


    setThreadId(id);
//_thread_pin(id);
    simSRandom(id + 1);
    simStackThreadStateInit(&th_state, id);

// Set the active_set vector to all zero.
    SHARED_OBJECT_INIT(id);

//Am I the last thread?
    if (id == N_THREADS - 1)
        d1 = getTimeMillis();

// Synchronization point
    int rc = pthread_barrier_wait(&barr);
    if (rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD) {
        printf("Could not wait on barrier\n");
        exit(EXIT_FAILURE);
    }
//start_cpu_counters(id);
    for (i = 0; i < RUNS; i++) {
        apply_op(&th_state, (ArgVal) PUSH, id);
    }
    // printf("%s\n", "=============");
    for (i = 0; i < RUNS; i++) {
        // apply_op(&th_state, (ArgVal) POP, id);
    }
//stop_cpu_counters(id);
}

inline static void* EntryPoint(void* Arg) {
    Execute(Arg);
    return null;
}
// This creates a thread and returns it ?
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

// This is just creating the thread, setting up the barrier and starting the therad.
// After te thread is done working it does some clean up operations also.
int main(int argc, char *argv[]) {
    pthread_t threads[N_THREADS];
    int i;
    init_cpu_counters();
    pthread_mutex_init(&printf_mutex, NULL);

    // if (argc != 3) {
    //     fprintf(stderr, "ERROR: Please provide N_THREADS and RUNS!. Argc count is : %d it should be 3.\n",argc);
    //     exit(EXIT_SUCCESS);
    // } 
    // else {
    //     sscanf(argv[1], "%d", &N_THREADS);
    //     sscanf(argv[2], "%d", &RUNS);
    // }


    printf(" N_THREADS is %d and RUNS is: %d\n", N_THREADS, RUNS);
    printf(" _TVEC_BIWORD_SIZE_ is %d\n", _TVEC_BIWORD_SIZE_);

#ifdef sparc
    printf("%s\n", "SPARC is defined");
#else
    printf("%s\n", "SPARC is NOT defined");
#endif

    if (pthread_barrier_init(&barr, NULL, N_THREADS)) {
        printf("Could not create the barrier\n");
        return -1;
    }

// This is actually creating the thread.
    for (i = 0; i < N_THREADS; i++)
        threads[i] = StartThread(i);

// Waiting of all the theads to finish there operation.
    for (i = 0; i < N_THREADS; i++)
        pthread_join(threads[i], NULL);

    d2 = getTimeMillis();

    countElements(false);
    printf("time: %d\t", (int) (d2 - d1));
// printStats();

    if (pthread_barrier_destroy(&barr)) {
        printf("Could not destroy the barrier\n");
        return -1;
    }
    return 0;
}
