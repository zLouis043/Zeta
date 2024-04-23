#include "Memory.h"

void mem_init(Memory * memory, size_t cap){
    memory->capacity = cap;
    memory->count = 0;
    memory->mem = (MemorySlot *)malloc(sizeof(MemorySlot) * cap);
}

MemorySlot memSlot(u8 byte, Type type, bool is_immutable){
    return (MemorySlot){
        .byte = byte,
        .type = type,
        .is_immutable = is_immutable
    };
}

bool mem_append(Memory * memory, MemorySlot slot){

    if(memory->count >= memory->capacity){
        return false;
    }

    memory->mem[memory->count++] = slot;

    return true;

}

bool mem_remove(Memory * memory, MemorySlot * ret){

    if(memory->count <= 0){
        return false;
    }

    memory->count--;

    *ret = memory->mem[memory->count];

    return true;

}

#include <stdio.h>

void print_memory(Memory * memory){

    for(size_t i = 0; i < memory->count; i++){
        printf("%zu] %02x %d\n", i, memory->mem[i].byte, memory->mem[i].type);
    }

    printf("\n");

}

void mem_free(Memory * memory){

    memory->count = 0;
    free(memory->mem);

}