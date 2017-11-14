

inline static void push(SimStackThreadState *th_state, ObjectState *st, ArgVal arg) {
    // printf("%s %p\n","st", st );
#ifdef DEBUG
    st->counter += 1;
#endif
    Node *n;
    n = alloc_obj(&th_state->pool_node);
    // printf("%s %p\n","n", n );
    n->value = (ArgVal)arg;
    n->next = st->head;
    if(st->head != null){
        // printf("%s %d\n","Head index is: ",(st->head)->index );
        n->index = (st->head)->index+1;
    }
    else{
        // printf("%s\n", "Head is null");
        n->index = 0;
    }
    // printf("%s %p\n","st->head", st->head );
    st->head = n;
    // printf("%s %p\n","st->head", st->head );

    // printf("%s %p\n","st", st );
}

inline static void pop(ObjectState *st, int pid) {
#ifdef DEBUG
    st->counter += 1;
#endif
    if(st->head != null) {
        st->ret[pid] = (RetVal)st->head->value;
        st->head = (Node *)st->head->next;
    }
    else st->ret[pid] = (RetVal)-1;
}