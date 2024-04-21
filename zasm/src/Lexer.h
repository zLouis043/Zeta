#ifndef ZASM_LEXER_H
#define ZASM_LEXER_H

#include "Linezer.h"
#include "Token.h"
#include "common/Configs.h"

#include <sys/time.h>

typedef struct TokenDA TokenDA;

struct TokenDA{
    Token * items;
    size_t count;
    size_t capacity;
};

typedef struct Lexer Lexer;

struct Lexer{
    SV filepath;
    LinesDA lda;
    TokenDA tda;
    Location curr;
    size_t start;
    struct timeval start_t, end_t;
};

void lexer_init(Lexer * lexer, char * filepath);
void lex(Lexer * lexer, Configs conf);

#endif //ZASM_LEXER_H
