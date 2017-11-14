// Struct to hold the data
typedef struct Node{
    volatile struct Node *next; // Pointer to the next node.
    Object value;               // Value to be stored.
    int index;
} Node;

// // This is the State which is mentioend in tje papers.
// typedef struct HalfObjectState {
//     ToggleVector applied;
//     Node *head;
//     Object ret[N_THREADS];
// #ifdef DEBUG
//     int counter;
// #endif
// } HalfObjectState;


// Then what is this thing.
// THis just contains a pad. THis will help in the making sure the cache coherence is protected.
typedef struct ObjectState {
    ToggleVector applied;
    Node *head;
    Object ret[N_THREADS];
#ifdef DEBUG
    int counter;
#endif
    //int32_t pad[PAD_CACHE(sizeof(ObjectState))];
} ObjectState;


// 48 bits for  the sequence number and then 16 bits for indexing into it.
typedef union pointer_t {
	struct StructData{
        int64_t seq : 48;
        int32_t index : 16;
	} struct_data;
	int64_t raw_data;
} pointer_t;

typedef struct SimStackThreadState {
    PoolStruct pool_node;
    ToggleVector mask;
    // ToggleVector CACHE_ALIGN;
    ToggleVector toggle;
    ToggleVector my_bit;
    int local_index;
    int backoff;
} SimStackThreadState;