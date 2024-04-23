#ifndef ZASM_STACK_H
#define ZASM_STACK_H

#include "common/ByteUtils.h"
#include "OpCode.h"
#include "common/DyArray.h"

typedef union AsSlot AsSlot;

union AsSlot{
    char char_;
    short short_;
    int int_;
    char * ptr_;
    double double_;
};

typedef struct StackSlot StackSlot;

struct StackSlot{
    AsSlot as;
    Type type;
};

typedef struct Stack Stack;

struct Stack{
    StackSlot * slots;
    size_t count;
    size_t capacity;
};

void stack_init(Stack * stack, size_t cap);
StackSlot stackSlot(AsSlot value, Type type);
bool push(Stack * stack, StackSlot slot);
bool pop(Stack * stack, StackSlot * ret);
void print_stack(Stack * stack);
void stack_free(Stack * stack);

#endif //ZASM_STACK_H
