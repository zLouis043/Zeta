#include "Token.h"

#include <stdio.h>

Location loc(SV filename, SV lineSV, size_t line, size_t col){
    return (Location){
        .filepath = filename,
        .lineSV = lineSV,
        .line = line,
        .column = col
    };
}

Token token(SV value, _TokenType type, Location loc){
    return (Token){
        .value = value,
        .type = type,
        .loc = loc,
        .is_expanded = false
    };
}

Token token_exp(SV value, _TokenType type, Location loc, LocDA expDA) {
    return (Token){
            .value = value,
            .type = type,
            .loc = loc,
            .expDA = expDA,
            .is_expanded = true
    };
}

#define TOSTR(type) case type: return #type

char * type_to_string(_TokenType type){
    switch(type){
        TOSTR(TT_COMMA);
        TOSTR(TT_COLON);
        TOSTR(TT_DOLLAR);
        TOSTR(TT_DOT);
        TOSTR(TT_OPEN_SQUARE);
        TOSTR(TT_CLOSE_SQUARE);
        TOSTR(TT_END);

        TOSTR(TT_IDENTIFIER);
        TOSTR(TT_STRING);
        TOSTR(TT_CHAR);
        TOSTR(TT_MEM_PTR);
        TOSTR(TT_PTR);
        TOSTR(TT_INTEGER);
        TOSTR(TT_DOUBLE);

        TOSTR(TT_HEX_TYPE);
        TOSTR(TT_CHAR_TYPE);
        TOSTR(TT_PTR_TYPE);
        TOSTR(TT_INTEGER_TYPE);
        TOSTR(TT_DOUBLE_TYPE);

        TOSTR(TT_INCLUDE);
        TOSTR(TT_DATA_START);
        TOSTR(TT_DATA_END);
        TOSTR(TT_CODE_START);
        TOSTR(TT_CODE_END);
        TOSTR(TT_EXPORT_START);
        TOSTR(TT_EXPORT_END);
        TOSTR(TT_ENTRY);

        TOSTR(TT_MOVE);
        TOSTR(TT_GET);
        TOSTR(TT_DEREF);
        TOSTR(TT_CAST);

        TOSTR(TT_PUSH);
        TOSTR(TT_PUSH_CHAR);
        TOSTR(TT_PUSH_INTEGER);
        TOSTR(TT_PUSH_DOUBLE);
        TOSTR(TT_PUSH_IN_PTR);
        TOSTR(TT_PUSH_PTR_REF);
        TOSTR(TT_PUSH_STRING);

        TOSTR(TT_ADD_CHAR);
        TOSTR(TT_SUB_CHAR);
        TOSTR(TT_MUL_CHAR);
        TOSTR(TT_DIV_CHAR);

        TOSTR(TT_ADD_INT);
        TOSTR(TT_SUB_INT);
        TOSTR(TT_MUL_INT);
        TOSTR(TT_DIV_INT);

        TOSTR(TT_ADD_DOUBLE);
        TOSTR(TT_SUB_DOUBLE);
        TOSTR(TT_MUL_DOUBLE);
        TOSTR(TT_DIV_DOUBLE);

        TOSTR(TT_CHAR_DEF);
        TOSTR(TT_INTEGER_DEF);
        TOSTR(TT_DOUBLE_DEF);
        TOSTR(TT_STRING_DEF);

        TOSTR(TT_DUMP);
        TOSTR(TT_EXIT);
        TOSTR(TT_CONCAT);
        TOSTR(TT_POP);
        TOSTR(TT_STACK_PRINT);
        TOSTR(TT_SWAP);

        TOSTR(TT_DUP);

        TOSTR(TT_LABEL);

        TOSTR(TT_JMP);
        TOSTR(TT_JMP_ZERO);
        TOSTR(TT_JMP_NOT_ZERO);
        TOSTR(TT_JMP_GREATER);
        TOSTR(TT_JMP_GREATER_EQUAL);
        TOSTR(TT_JMP_LESS);
        TOSTR(TT_JMP_LESS_EQUAL);
        TOSTR(TT_JMP_EQUAL);
        TOSTR(TT_JMP_NOT_EQUAL);

        TOSTR(TT_CONST_DEF);
        TOSTR(TT_CONST_LABEL);
        TOSTR(TT_CONSTANT);

        TOSTR(TT_COMPARE);
        TOSTR(TT_COMPARE_CHAR);
        TOSTR(TT_COMPARE_INTEGER);
        TOSTR(TT_COMPARE_DOUBLE);
        TOSTR(TT_COMPARE_PTR_REF);
        TOSTR(TT_DROP);

        TOSTR(TT_CALL);
        TOSTR(TT_RET);
        TOSTR(TT_R1);
        TOSTR(TT_R2);
        TOSTR(TT_R3);
        TOSTR(TT_R4);
        TOSTR(TT_R5);
        TOSTR(TT_R6);
        TOSTR(TT_R7);
        TOSTR(TT_R8);
        TOSTR(TT_CLEAN);

        TOSTR(TT_MACRO_DEF);
        TOSTR(TT_MACRO_END);

        TOSTR(TT_SHIFT_LEFT);
        TOSTR(TT_SHIFT_RIGHT);
        TOSTR(TT_AND);
        TOSTR(TT_OR);
        TOSTR(TT_NOT);

        TOSTR(TT_MEMORY);
        TOSTR(TT_STORE);
        TOSTR(TT_INTERNAL);

        TOSTR(TT_END_INCLUDE);
        TOSTR(TT_EOF);
    }
    return "";
}

char * tok_to_string(Token token){

    char * buffer = malloc(1024);
    sprintf(buffer, "\"%-22s\" | %-15s | %s:%zu:%zu", CSTR(token.value), type_to_string(token.type), CSTR(token.loc.filepath),
            token.loc.line+1, token.loc.column);

    return buffer;
}