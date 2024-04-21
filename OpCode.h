#ifndef ZASM_OPCODE_H
#define ZASM_OPCODE_H

#include "common/ByteUtils.h"
#include "common/CommonStructs.h"
#include "Token.h"

typedef enum OPCODES OPCODES;

enum OPCODES{
    OP_PUSH_C, OP_PUSH_I, OP_PUSH_D, OP_PUSH_PR, OP_PUSH_P, OP_PUSH_STR,
    OP_ADD_C, OP_SUB_C, OP_MUL_C, OP_DIV_C,
    OP_ADD_I, OP_SUB_I, OP_MUL_I, OP_DIV_I,
    OP_ADD_D, OP_SUB_D, OP_MUL_D, OP_DIV_D,
    OP_DUP,
    OP_CONST_CHAR_DEF, OP_CONST_INTEGER_DEF, OP_CONST_DOUBLE_DEF,
    OP_VAR_DEF, OP_STRING_DEF, OP_STRING_TAIL, OP_MEM_DEF,
    OP_ENTRY, OP_JMP, OP_JMP_E, OP_JMP_NE, OP_JMP_G, OP_JMP_GE, OP_JMP_L, OP_JMP_LE, OP_JMP_Z, OP_JMP_NZ,
    OP_CMP_C, OP_CMP_PR, OP_CMP_I, OP_CMP_D,
    OP_CALL, OP_RET,
    OP_DEREF,
    OP_MOVE, OP_GET, OP_CLEAN,
    OP_CAST, OP_DROP, OP_DUMP, OP_EXIT, OP_SWAP,
    OP_STORE,
    OP_SHL, OP_SHR, OP_AND, OP_OR, OP_NOT,
    OP_NOP, OP_STACKPRINT, OP_INTERNAL, OP_END,
};

typedef enum Type Type;

enum Type{
    NONE_,
    BYTE_,
    CHAR_,
    SHORT_,
    IN_PTR_,
    INTEGER_,
    PTR_,
    DOUBLE_,
    BYTE_ARRAY_,
};

typedef union As As;

union As{
    u8 one;
    short two;
    int four;
    double eight;
    ByteDA bda;
};

typedef struct OPCODE OPCODE;

struct OPCODE{
    OPCODES op;
    As as;
    Type t;
    Location loc;
    bool is_expanded;
    LocDA expDA;
};

#define non (As){.one = 0}

OPCODE opcode(OPCODES op, As val, Type type, Token tok);
char * op_to_str(OPCODE op);
char * opcode_to_str(int op);
char * type_to_str(Type type);
void op_print(OPCODE op);
char * opcode_to_code(int op);

#endif //ZASM_OPCODE_H
