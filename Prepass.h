#ifndef ZASM_PREPASS_H
#define ZASM_PREPASS_H

#include "Lexer.h"
#include "common/Configs.h"
#include "common/DyArray.h"
#include "common/Hashmap.h"

typedef struct Block Block;

struct Block{
    Token * items;
    size_t count;
    size_t capacity;
};

typedef struct Arg Arg;

struct Arg{
    SV key;
};

typedef struct ArgDA ArgDA;

struct ArgDA{
    Arg * items;
    size_t count;
    size_t capacity;
};

typedef struct SpecialKeyWord SpecialKeyWord;

struct SpecialKeyWord{
    SV key;
    size_t id;
};

typedef struct Macro Macro;

struct Macro{
    SV key;
    Block value;
    ArgDA argDa;
};

typedef struct Expanded_Include Expanded_Include;

struct Expanded_Include{
    SV key;
    bool value;
};

typedef struct Include Include;

struct Include{
    SV key;
    Block value;
};

typedef struct FilesDA FilesDA;

struct FilesDA{
    SV * items;
    size_t count;
    size_t capacity;
};

typedef struct Prepass Prepass;

struct Prepass{

    Lexer lexer_base;
    bool lexer_base_set;
    bool can_expand;
    bool is_head_label_set;
    SV head_label;
    Lexer lexer_curr;
    TokenDA tda;
    SV filepath;
    struct hashmap * macros;
    struct hashmap * special_key_words;
    struct hashmap * includes;
    struct hashmap * expanded_includes;
    size_t curr_macro_depth;
    size_t curr_include_depth;
    Token curr;
    FilesDA fda;
    Block temp_block;
    LocDA expansionDA;
};

void prepass_init(Prepass * pp);
void preprocess(Prepass * pp, Configs conf);

#endif //ZASM_PREPASS_H
