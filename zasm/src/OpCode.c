#include "OpCode.h"

OPCODE opcode(OPCODES op, As val, Type type, Token tok){
    return (OPCODE){
        .op = op,
        .as = val,
        .t = type,
        .loc = tok.loc,
        .is_expanded = tok.is_expanded,
        .expDA = tok.expDA
    };
}

char * opcode_to_code(int op) {
    switch (op) {

        case OP_ENTRY:
            return "ENTRY";
        case OP_MOVE:
            return "move";
        case OP_GET:
            return "get";
        case OP_DEREF:
            return "deref";
        case OP_CAST:
            return "cast";

        case OP_PUSH_C:
            return "pushC";
        case OP_PUSH_I:
            return "pushI";
        case OP_PUSH_D:
            return "pushD";
        case OP_PUSH_P:
            return "pushP";
        case OP_PUSH_PR:
            return "pushPR";
        case OP_PUSH_STR:
            return "pushSTR";

        case OP_ADD_C:
            return "addC";
        case OP_SUB_C:
            return "subC";
        case OP_MUL_C:
            return "mulC";
        case OP_DIV_C:
            return "divC";

        case OP_ADD_I:
            return "addI";
        case OP_SUB_I:
            return "subI";
        case OP_MUL_I:
            return "mulI";
        case OP_DIV_I:
            return "divI";

        case OP_ADD_D:
            return "addD";
        case OP_SUB_D:
            return "subD";
        case OP_MUL_D:
            return "mulD";
        case OP_DIV_D:
            return "divD";

        case OP_STRING_DEF:
            return "STRING";
        case OP_MEM_DEF:
            return "MEMORY";
        case OP_STORE:
            return "store";

        case OP_DUMP:
            return "dump";
        case OP_EXIT:
            return "exit";

        case OP_DUP:
            return "dup";

        case OP_JMP:
            return "jmp";
        case OP_JMP_Z:
            return "jmp_z";
        case OP_JMP_NZ:
            return "jmp_nz";
        case OP_JMP_G:
            return "jmp_g";
        case OP_JMP_GE:
            return "jmp_ge";
        case OP_JMP_L:
            return "jmp_l";
        case OP_JMP_LE:
            return "jmp_le";
        case OP_JMP_E:
            return "jmp_e";
        case OP_JMP_NE:
            return "jmp_ne";

        case OP_CONST_CHAR_DEF:
        case OP_CONST_INTEGER_DEF:
        case OP_CONST_DOUBLE_DEF:
            return "CONST";

        case OP_CMP_C:
            return "cmpC";
        case OP_CMP_PR:
            return "cmpPR";
        case OP_CMP_I:
            return "cmpI";
        case OP_CMP_D:
            return "cmpD";

        case OP_DROP:
            return "drop";

        case OP_SHL:
            return "shl";
        case OP_SHR:
            return "shr";
        case OP_AND:
            return "and";
        case OP_OR:
            return "or";
        case OP_NOT:
            return "not";

        case OP_CALL:
            return "call";
        case OP_RET:
            return "ret";
        case OP_CLEAN:
            return "clean";
        case OP_NOP:
            return "nop";
        case OP_STACKPRINT:
            return "stack_print";
        case OP_SWAP:
            return "swap";
        case OP_INTERNAL:
            return "internal";
        case OP_END:
            return "end";
    }
}

#define TOSTR(type) case type: return #type

char * opcode_to_str(int op){
    switch(op){

        TOSTR(OP_ENTRY);

        TOSTR(OP_MOVE);
        TOSTR(OP_GET);
        TOSTR(OP_DEREF);
        TOSTR(OP_CAST);

        TOSTR(OP_PUSH_C);
        TOSTR(OP_PUSH_I);
        TOSTR(OP_PUSH_D);
        TOSTR(OP_PUSH_PR);
        TOSTR(OP_PUSH_STR);

        TOSTR(OP_ADD_C);
        TOSTR(OP_SUB_C);
        TOSTR(OP_MUL_C);
        TOSTR(OP_DIV_C);

        TOSTR(OP_ADD_I);
        TOSTR(OP_SUB_I);
        TOSTR(OP_MUL_I);
        TOSTR(OP_DIV_I);

        TOSTR(OP_ADD_D);
        TOSTR(OP_SUB_D);
        TOSTR(OP_MUL_D);
        TOSTR(OP_DIV_D);

        TOSTR(OP_STRING_DEF);
        TOSTR(OP_STRING_TAIL);
        TOSTR(OP_MEM_DEF);
        TOSTR(OP_STORE);

        TOSTR(OP_DUMP);
        TOSTR(OP_EXIT);

        TOSTR(OP_DUP);

        TOSTR(OP_JMP);
        TOSTR(OP_JMP_Z);
        TOSTR(OP_JMP_NZ);
        TOSTR(OP_JMP_G);
        TOSTR(OP_JMP_GE);
        TOSTR(OP_JMP_L);
        TOSTR(OP_JMP_LE);
        TOSTR(OP_JMP_E);
        TOSTR(OP_JMP_NE);

        TOSTR(OP_CONST_CHAR_DEF);
        TOSTR(OP_CONST_INTEGER_DEF);
        TOSTR(OP_CONST_DOUBLE_DEF);

        TOSTR(OP_CMP_C);
        TOSTR(OP_CMP_PR);
        TOSTR(OP_CMP_I);
        TOSTR(OP_CMP_D);

        TOSTR(OP_DROP);

        TOSTR(OP_SHL);
        TOSTR(OP_SHR);
        TOSTR(OP_AND);
        TOSTR(OP_OR);
        TOSTR(OP_NOT);

        TOSTR(OP_CALL);
        TOSTR(OP_RET);
        TOSTR(OP_CLEAN);
        TOSTR(OP_NOP);
        TOSTR(OP_STACKPRINT);
        TOSTR(OP_SWAP);
        TOSTR(OP_INTERNAL);
        TOSTR(OP_END);

    }
    return NULL;
}

char * op_to_str(OPCODE op){
    switch(op.op){
        
        TOSTR(OP_ENTRY);

        TOSTR(OP_MOVE);
        TOSTR(OP_GET);
        TOSTR(OP_DEREF);
        TOSTR(OP_CAST);

        TOSTR(OP_PUSH_C);
        TOSTR(OP_PUSH_I);
        TOSTR(OP_PUSH_D);
        TOSTR(OP_PUSH_PR);
        TOSTR(OP_PUSH_P);
        TOSTR(OP_PUSH_STR);

        TOSTR(OP_ADD_C);
        TOSTR(OP_SUB_C);
        TOSTR(OP_MUL_C);
        TOSTR(OP_DIV_C);

        TOSTR(OP_ADD_I);
        TOSTR(OP_SUB_I);
        TOSTR(OP_MUL_I);
        TOSTR(OP_DIV_I);

        TOSTR(OP_ADD_D);
        TOSTR(OP_SUB_D);
        TOSTR(OP_MUL_D);
        TOSTR(OP_DIV_D);

        TOSTR(OP_STRING_DEF);
        TOSTR(OP_STRING_TAIL);
        TOSTR(OP_MEM_DEF);
        TOSTR(OP_STORE);

        TOSTR(OP_DUMP);
        TOSTR(OP_EXIT);

        TOSTR(OP_DUP);

        TOSTR(OP_JMP);
        TOSTR(OP_JMP_Z);
        TOSTR(OP_JMP_NZ);
        TOSTR(OP_JMP_G);
        TOSTR(OP_JMP_GE);
        TOSTR(OP_JMP_L);
        TOSTR(OP_JMP_LE);
        TOSTR(OP_JMP_E);
        TOSTR(OP_JMP_NE);

        TOSTR(OP_CONST_CHAR_DEF);
        TOSTR(OP_CONST_INTEGER_DEF);
        TOSTR(OP_CONST_DOUBLE_DEF);

        TOSTR(OP_CMP_C);
        TOSTR(OP_CMP_PR);
        TOSTR(OP_CMP_I);
        TOSTR(OP_CMP_D);

        TOSTR(OP_DROP);

        TOSTR(OP_SHL);
        TOSTR(OP_SHR);
        TOSTR(OP_AND);
        TOSTR(OP_OR);
        TOSTR(OP_NOT);

        TOSTR(OP_CALL);
        TOSTR(OP_RET);
        TOSTR(OP_CLEAN);
        TOSTR(OP_NOP);
        TOSTR(OP_STACKPRINT);
        TOSTR(OP_INTERNAL);
        TOSTR(OP_SWAP);
        TOSTR(OP_END);
        case OP_VAR_DEF:
            break;
    }
    return NULL;
}

char * type_to_str(Type type){
    switch(type){
        TOSTR(CHAR_);
        TOSTR(PTR_);
        TOSTR(INTEGER_);
        TOSTR(DOUBLE_);
        case NONE_:
            break;
        case BYTE_:
            break;
        case SHORT_:
            break;
        case IN_PTR_:
            break;
        case BYTE_ARRAY_:
            break;
    }
    return NULL;
}

#include <stdio.h>

void op_print(OPCODE op){

    printf("%-20s | ", op_to_str(op));

    switch(op.t){
        case NONE_: printf("%*.s |", 23, " "); break;
        case BYTE_: {
            u8 * arr = (u8*)(&op.as.one);

            for(size_t i = 0; i < sizeof(u8); i++){
                printf("%02x ", arr[i]);
            }

            for(size_t i = 0; i < 8 - sizeof(u8); i++){
                printf("00 ");
            }

            printf("|");

        }break;
        case CHAR_: printf("%23c |", op.as.one); break;
        case SHORT_: {
            u8 * arr = (u8*)(&op.as.two);

            for(size_t i = 0; i < sizeof(short); i++){
                printf("%02x ", arr[i]);
            }

            for(size_t i = 0; i < 8 - sizeof(short); i++){
                printf("00 ");
            }

            printf("|");
        }break;
        case INTEGER_:{
            u8 * arr = (u8*)(&op.as.four);

            for(size_t i = 0; i < sizeof(int); i++){
                printf("%02x ", arr[i]);
            }

            for(size_t i = 0; i < 8 - sizeof(int); i++){
                printf("00 ");
            }

            printf("|");
        }break;
        case DOUBLE_:{
            u8 * arr = (u8*)(&op.as.eight);

            for(size_t i = 0; i < sizeof(double ); i++){
                printf("%02x ", arr[i]);
            }

            printf("|");
        }break;
        case BYTE_ARRAY_:{
            for(size_t i = 0; i < op.as.bda.count; i++){
                printf("%02x ", op.as.bda.items[i]);
            }

            printf("|");
        }
        case IN_PTR_:
            break;
        case PTR_:
            break;
    }

    printf("  %s:%zu:%zu\n", CSTR(op.loc.filepath),
           op.loc.line+1, op.loc.column);

}
