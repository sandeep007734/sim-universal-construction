static inline int countElements(bool print){
    pointer_t new_sp, old_sp;
    ObjectState *lsp_data, *sp_data;

    old_sp = sp;
    sp_data = (ObjectState *)&pool[old_sp.struct_data.index];    
    volatile Node *node = sp_data->head;
    int count=0;
    while(node != NULL){
        count++;
        if(print)
        printf("%d, ", node->index);    
        node = node->next;
    }
    if(print)
    printf("%s\n", "");
    printf("%s %d\n", "Total Elements: ",count);
    
}

static pthread_mutex_t printf_mutex;

int sync_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    pthread_mutex_lock(&printf_mutex);
    vprintf(format, args);
    pthread_mutex_unlock(&printf_mutex);

    va_end(args);
}
