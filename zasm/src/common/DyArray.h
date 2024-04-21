#ifndef ZASM_DYARRAY_H
#define ZASM_DYARRAY_H

#define DA_INIT_CAP 16

#define da_push(da, item) \
    do{                                     \
        if(da.count >= da.capacity){    \
            size_t new_cap = da.capacity;\
            if(new_cap == 0){\
                new_cap = DA_INIT_CAP;\
            }else{\
                new_cap *= 2;\
            }\
            da.items = realloc(da.items, new_cap * sizeof(*da.items));\
            if(da.items == NULL){\
                fprintf(stderr, "Failed to updated da");\
                exit(-1);\
            }\
            da.capacity = new_cap;\
            }\
            da.items[da.count++] = item;                      \
    }while(0)

#endif //ZASM_DYARRAY_H
