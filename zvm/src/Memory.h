#ifndef ZASM_MEMORY_H
#define ZASM_MEMORY_H

#include "common/ByteUtils.h"
#include "OpCode.h"

typedef struct MemorySlot MemorySlot;

struct MemorySlot{
    u8 byte;
    Type type;
    bool is_immutable;
};

typedef struct Memory Memory;

struct Memory{
    MemorySlot * mem;
    size_t count;
    size_t capacity;
};

void mem_init(Memory * memory, size_t cap);
MemorySlot memSlot(u8 byte, Type type, bool is_immutable);
bool mem_append(Memory * memory, MemorySlot slot);
bool mem_remove(Memory * memory, MemorySlot * ret);
void print_memory(Memory * memory);
void mem_free(Memory * memory);

#endif //ZASM_MEMORY_H
