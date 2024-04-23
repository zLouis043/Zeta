#ifndef ZASM_VM_H
#define ZASM_VM_H

#include "Stack.h"
#include "Memory.h"
#include "common/Configs.h"


#include <stdio.h>

typedef struct OpInfo OpInfo;

struct OpInfo{
    u8 file_idx;
    short line;
    short col;
};

typedef struct OpInfoDA OpInfoDA;

struct OpInfoDA{
    OpInfo * items;
    size_t count;
    size_t capacity;
};

typedef struct FilesDA FilesDA;

struct FilesDA{
    char ** items;
    size_t count;
    size_t capacity;
};

typedef struct Register Register;

struct Register{
    AsSlot value;
    Type type;
};

typedef struct Op Op;

struct Op{
    OPCODES opcode;
    Location loc;
    As as;
    bool is_extended;
    LocDA exd_da_ext;
    Type type;
};

typedef struct ChunkDA ChunkDA;

struct ChunkDA{
    Op * items;
    size_t count;
    size_t capacity;
};

typedef struct VM VM;

struct VM{
    Configs conf;
    FILE* stream;
    SV org_filepath;
    FilesDA fda;
    ChunkDA dataDA;
    ChunkDA codeDA;
    Stack runtime_stack;
    Stack call_stack;
    Memory memory;
    Register regs[8];
    uint8_t flags;
    int exit_value;
    size_t ip;
    size_t data_chunk_size;
    size_t code_chunk_size;
    size_t entry_point;
    bool has_dbg_info;
    bool end_program;

    // 0 0 0 0 0 0 0 0
    //         E G L Z
    // EQUAL
    // GREATER
    // LESS
    // ZERO
};

void vm_init(VM *vm, char * filepath, Configs conf);
void vm_gather(VM * vm);
int vm_run(VM * vm);
void vm_disassemble(VM * vm, bool print_file_info);

#endif //ZASM_VM_H
