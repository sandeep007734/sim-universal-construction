

inline static void push(SimStackThreadState *th_state, ObjectState *st, ArgVal arg) {
#ifdef DEBUG
    st->counter += 1;
#endif
    Node *n;
    n = alloc_obj(&th_state->pool_node);
    n->value = (ArgVal)arg;
    n->next = st->head;
    st->head = n;
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