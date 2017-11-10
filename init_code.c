#include "shared_structs.c"



// Shared variables
static int MAX_BACK = 0;
volatile pointer_t sp ;
volatile ToggleVector a_toggles ;
// _TVEC_BIWORD_SIZE_ is a performance workaround for 
// array announce. Size N_THREADS is algorithmically enough.
volatile ArgVal announce[N_THREADS + _TVEC_BIWORD_SIZE_] ;
volatile ObjectState pool[LOCAL_POOL_SIZE * N_THREADS + 1] ;



void simStackThreadStateInit(SimStackThreadState *th_state, int pid) {
    th_state->local_index = 0;
    TVEC_SET_ZERO(&th_state->mask);
    TVEC_SET_ZERO(&th_state->my_bit);
    TVEC_SET_ZERO(&th_state->toggle);
    TVEC_REVERSE_BIT(&th_state->my_bit, pid);
    TVEC_SET_BIT(&th_state->mask, pid);
    th_state->toggle = TVEC_NEGATIVE(th_state->mask);
    th_state->backoff = 1;

    init_pool(&th_state->pool_node, sizeof(Node));
}


void SHARED_OBJECT_INIT(int pid) {
    if (pid == 0) {
        sp.struct_data.index = LOCAL_POOL_SIZE * N_THREADS;
        sp.struct_data.seq = 0;
        TVEC_SET_ZERO((ToggleVector *)&a_toggles);

        // OBJECT'S INITIAL VALUE
        // ----------------------
        pool[LOCAL_POOL_SIZE * N_THREADS].head = null;
        TVEC_SET_ZERO((ToggleVector *)&pool[LOCAL_POOL_SIZE * N_THREADS].applied);

    }
}