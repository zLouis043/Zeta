#include "Stack.h"

#include "common/Log.h"
#include <stdio.h>

void stack_init(Stack * stack, size_t cap){

    stack->capacity = cap;
    stack->count = 0;
    stack->slots = (StackSlot *)malloc(sizeof(StackSlot) * cap);

}

StackSlot stackSlot(AsSlot value, Type type){
    return (StackSlot){
        .as = value,
        .type = type
    };
}

bool push(Stack * stack, StackSlot slot){

    if(stack->count >= stack->capacity){
        return false;
    }

    stack->slots[stack->count++] = slot;

    return true;
}

bool pop(Stack * stack, StackSlot * ret){

    if(stack->count <= 0){
        return false;
    }

    stack->count--;

    *ret = stack->slots[stack->count];

    return true;

}

void print_stack(Stack * stack){

    for(size_t i = 0; i < stack->count; i++){
        AsSlot value = stack->slots[i].as;
        Type t = stack->slots[i].type;
        if(t == DOUBLE_){
            printf("%zu] %0.13g %d\n", i, value.double_, t);
        }else if(t == INTEGER_){
            printf("%zu] %d %d\n", i, value.int_, t);
        }else if(t == CHAR_){
            printf("%zu] %c %d\n", i, value.char_, t);
        }else if(t == IN_PTR_){
            printf("%zu] 0x%hd %d\n", i, value.short_, t);
        }else if(t == PTR_){
            printf("%zu] 0x%p %d\n", i, value.ptr_, t);
        }
    }

    printf("\n");
}

void stack_free(Stack * stack){

    stack->count = 0;
    free(stack->slots);

}