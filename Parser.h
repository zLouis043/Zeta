#ifndef ZASM_PARSER_H
#define ZASM_PARSER_H

#include "common/ByteUtils.h"
#include "common/Hashmap.h"
#include "common/Configs.h"
#include "OpCode.h"
#include "Prepass.h"

typedef struct ChunkDA ChunkDA;

struct ChunkDA{
    OPCODE * items;
    size_t count;
    size_t capacity;
};

typedef struct Label Label;

struct Label{
    SV key;
    size_t value;
};

typedef struct MemoryPtr MemoryPtr;

struct MemoryPtr{
    SV key;
    size_t value;
};

typedef struct IdentTok IdentTok;

struct IdentTok{
    SV key;
    Token tok;
};

typedef struct ReqLabelsDA ReqLabelsDA;

struct ReqLabelsDA{
    Label * items;
    size_t count;
    size_t capacity;
};

typedef struct Parser Parser;

struct Parser{

    Token curr;
    size_t curr_idx;
    size_t curr_ptr;
    size_t base_addr;
    size_t data_addr;
    size_t code_addr;
    size_t curr_addr;
    size_t entry;
    TokenDA tda;
    FilesDA fda;
    ChunkDA code_chunk_da;
    ChunkDA data_chunk_da;
    ReqLabelsDA req_labels_da;
    bool is_entry_point_set;
    SV entry_point_key;
    struct hashmap * jmp_labels;
    struct hashmap * memory_locations;
    struct hashmap * identifiers;
    Configs conf;
    int err_code;
};

void parser_init(Parser * parser, Configs conf);
int parse(Parser * parser);

#endif //ZASM_PARSER_H
