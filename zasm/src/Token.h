#ifndef ZASM_TOKEN_H
#define ZASM_TOKEN_H

#include "common/SV.h"
#include <stdbool.h>
#include "common/CommonStructs.h"

typedef enum _TokenType _TokenType;

enum _TokenType{
    TT_COLON,
    TT_COMMA,
    TT_DOLLAR,
    TT_DOT,
    TT_OPEN_SQUARE,
    TT_CLOSE_SQUARE,
    TT_END,

    TT_STRING,
    TT_IDENTIFIER,

    TT_HEX_TYPE,
    TT_CHAR_TYPE,
    TT_PTR_TYPE,
    TT_INTEGER_TYPE,
    TT_DOUBLE_TYPE,

    TT_CHAR,
    TT_MEM_PTR,
    TT_PTR,
    TT_INTEGER,
    TT_DOUBLE,

    TT_INCLUDE,
    TT_DATA_START,
    TT_DATA_END,
    TT_CODE_START,
    TT_CODE_END,
    TT_EXPORT_START,
    TT_EXPORT_END,
    TT_ENTRY,

    TT_MOVE,
    TT_GET,
    TT_DEREF,
    TT_CAST,

    TT_PUSH,
    TT_PUSH_CHAR,
    TT_PUSH_INTEGER,
    TT_PUSH_DOUBLE,
    TT_PUSH_IN_PTR,
    TT_PUSH_PTR_REF,
    TT_PUSH_STRING,

    TT_ADD_CHAR,
    TT_SUB_CHAR,
    TT_MUL_CHAR,
    TT_DIV_CHAR,

    TT_ADD_INT,
    TT_SUB_INT,
    TT_MUL_INT,
    TT_DIV_INT,

    TT_ADD_DOUBLE,
    TT_SUB_DOUBLE,
    TT_MUL_DOUBLE,
    TT_DIV_DOUBLE,

    TT_CHAR_DEF,
    TT_INTEGER_DEF,
    TT_DOUBLE_DEF,
    TT_STRING_DEF,

    TT_DUMP,
    TT_EXIT,
    TT_CONCAT,
    TT_POP,
    TT_STACK_PRINT,
    TT_SWAP,

    TT_DUP,
    TT_LABEL,

    TT_JMP,
    TT_JMP_ZERO,
    TT_JMP_NOT_ZERO,
    TT_JMP_GREATER,
    TT_JMP_GREATER_EQUAL,
    TT_JMP_LESS,
    TT_JMP_LESS_EQUAL,
    TT_JMP_EQUAL,
    TT_JMP_NOT_EQUAL,

    TT_CONST_DEF,
    TT_CONST_LABEL,
    TT_CONSTANT,

    TT_COMPARE,
    TT_COMPARE_CHAR,
    TT_COMPARE_INTEGER,
    TT_COMPARE_DOUBLE,
    TT_COMPARE_PTR_REF,
    TT_DROP,

    TT_CALL,
    TT_RET,
    TT_R1,
    TT_R2,
    TT_R3,
    TT_R4,
    TT_R5,
    TT_R6,
    TT_R7,
    TT_R8,
    TT_CLEAN,

    TT_MACRO_DEF,
    TT_MACRO_END,

    TT_SHIFT_LEFT,
    TT_SHIFT_RIGHT,
    TT_AND,
    TT_OR,
    TT_NOT,

    TT_MEMORY,
    TT_STORE,

    TT_END_INCLUDE,
    TT_INTERNAL,
    TT_EOF
};

typedef struct Token Token;

struct Token{
    SV value;
    _TokenType type;
    Location loc;
    bool is_expanded;
    LocDA expDA;
};

Location loc(SV filename, SV lineSV, size_t line, size_t col);
Token token(SV value, _TokenType type, Location loc);
Token token_exp(SV value, _TokenType type, Location loc, LocDA expDA);
char * type_to_string(_TokenType type);
char * tok_to_string(Token token);

#endif //ZASM_TOKEN_H
