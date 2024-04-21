#include "Parser.h"

#include "common/Log.h"
#include <stdio.h>
#include <time.h>

#define PANIC_EXPAND_TOK(LOG_LEVEL, TOK, ...) \
    log_(LOG_LEVEL, &TOK.loc, __VA_ARGS__);                                         \
    if(TOK.is_expanded){                      \
    for(size_t z = 0; z < TOK.expDA.count; z++){ \
    log_(INFO, &TOK.expDA.items[z], "Macro expanded here");                                          \
    }                                          \
    }\
    parser->err_code = -1;

int Label_compare(const void *a, const void *b, void *udata) {
    (void)(udata);
    const Label *ua = a;
    const Label *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t Label_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Label *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}


int ID_compare(const void *a, const void *b, void *udata) {
    (void)(udata);
    const IdentTok *ua = a;
    const IdentTok *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t ID_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const IdentTok *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

int MemPtr_compare(const void *a, const void *b, void *udata) {
    (void)(udata);
    const MemoryPtr *ua = a;
    const MemoryPtr *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t MemPtr_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Include *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

#define code_advance(parser) (parser->code_addr++)

void parser_init(Parser * parser, Configs conf){

    Prepass pp = {0};

    prepass_init(&pp);

    preprocess(&pp, conf);

    if(pp.tda.count == 0){
        PANIC(GENERIC_ERROR, NULL, "File %.*s is empty\n", conf.filepath.len, conf.filepath.data);
    }

    parser->tda = pp.tda;
    parser->fda = pp.fda;
    parser->conf = conf;

    parser->entry_point_key.data = NULL;
    parser->entry_point_key.len = 0;

    parser->curr_idx = 0;
    parser->curr_ptr = 0;
    parser->err_code = 0;
    parser->base_addr += 5; // ZEXEC TAG
    parser->base_addr += 1; // DBG
    if(conf.is_dgb) {
        parser->base_addr += 8; // FILES COUNT
        for (size_t i = 0; i < parser->fda.count; i++) {
            parser->base_addr += 8; // FILE LEN
            parser->base_addr += parser->fda.items[i].len; // FILE STRING
        }
    }
    parser->base_addr += 8; // MEM_CAP
    parser->base_addr += 8; // STACK_CAP
    parser->base_addr += 10; // DATA_TAG
    parser->base_addr += 8; // DATA_COUNT
    parser->base_addr += 8; // ENTRY
    parser->base_addr += 10; // CODE_TAG
    parser->base_addr += 8; // CODE_SIZE
    parser->entry = 0;

    parser->is_entry_point_set = false;

    parser->jmp_labels = hashmap_new(sizeof(Label), 0, 0, 0,
                                Label_hash, Label_compare, NULL, NULL);
    parser->identifiers = hashmap_new(sizeof(IdentTok), 0, 0, 0,
                                           ID_hash, ID_compare, NULL, NULL);
    parser->memory_locations = hashmap_new(sizeof(MemoryPtr), 0, 0, 0,
                                           MemPtr_hash, MemPtr_compare, NULL, NULL);

#if 0
    FILE * fp = fopen("log_lex.txt", "w");

    if(!fp) {
        log(GENERIC_ERROR, NULL, "Could not create log file for lexing.");
    }

    fprintf(stdout, "%3zu |   %-22s  | %-15s | FILE_INFO\n", 0 , "TEXT VALUE", "TOK_TYPE");
    for(int i = 0; i < 83; i++){
        fprintf(stdout, "=");
    }
    fprintf(stdout, "\n");

    for(size_t i = 0; i < parser->tda.count; i++){\
        fprintf(stdout, "%3zu |  %s\n", i+1, tok_to_string(parser->tda.items[i]));\
    }

    fclose(fp);
#endif // LEX_DUMP
}

static bool isAtEnd(Parser * parser){
    return parser->curr_idx == parser->tda.count;
}


static Token advance(Parser * parser){

    parser->curr_idx++;

    return parser->tda.items[parser->curr_idx-1];

}

static Token peek(Parser * parser){
    if(isAtEnd(parser)) return token(SV("0"), TT_EOF, parser->curr.loc);
    return parser->tda.items[parser->curr_idx-1];
}

static bool match(Parser * parser, _TokenType expected){
    if(isAtEnd(parser)) return false;
    if(parser->tda.items[parser->curr_idx].type != expected) return false;

    parser->curr_idx++;
    return true;
}

static void checkIfEntryPoint(Parser * parser, SV identifier){

    if (sv_eq(identifier, parser->entry_point_key) == 0 && !parser->is_entry_point_set) {
        parser->entry = parser->code_addr;
        parser->is_entry_point_set = true;
    }
}

static ByteDA formatString(Parser * parser, ByteDA bda){
    ByteDA fmt = {0};

    for(size_t i = 0; i < bda.count; i++){
        if(bda.items[i] == '\\' && i + 1 < bda.count){
            i++;
            u8 byte = bda.items[i];
            switch(byte){
                case 'a':
                    da_push(fmt, (u8)7);
                    break;
                case 'b':
                    da_push(fmt, (u8)8);
                    break;
                case 't':
                    da_push(fmt, (u8)9);
                    break;
                case 'n':
                    da_push(fmt, (u8)10);
                    break;
                case 'r':
                    da_push(fmt, (u8)13);
                    break;
                case '0':
                    da_push(fmt, (u8)0);
                case '\'':
                    da_push(fmt, (u8)'\'');
                    break;
                case '\"':
                    da_push(fmt, (u8)'"');
                    break;
                case '\\':
                    da_push(fmt, (u8)'\\');
                    break;
                default:
                    da_push(fmt, (u8)' ');
                    break;
            }
        }else{
            da_push(fmt, bda.items[i]);
        }
    }
    return fmt;
}

static void emitString(Parser * parser, ByteDA bda, Token org){
    ByteDA fmt = formatString(parser, bda);

    /*for(size_t i = 0; i < bda.count; i++){
        if(bda.items[i] == '\\' && i + 1 < bda.count){
            i++;
            u8 byte = bda.items[i];
            switch(byte){
                case 'a':
                    da_push(fmt, (u8)7);
                    break;
                case 'b':
                    da_push(fmt, (u8)8);
                    break;
                case 't':
                    da_push(fmt, (u8)9);
                    break;
                case 'n':
                    da_push(fmt, (u8)10);
                    break;
                case 'r':
                    da_push(fmt, (u8)13);
                    break;
                case '0':
                    da_push(fmt, (u8)0);
                case '\'':
                    da_push(fmt, (u8)'\'');
                    break;
                case '\"':
                    da_push(fmt, (u8)'"');
                    break;
                case '\\':
                    da_push(fmt, (u8)'\\');
                    break;
                default:
                    da_push(fmt, (u8)' ');
                    break;
            }
        }else{
            da_push(fmt, bda.items[i]);
        }
    }*/

    parser->curr_ptr += 8;
    da_push(parser->data_chunk_da, opcode(OP_STRING_DEF, (As){.bda = fmt}, BYTE_ARRAY_, org));
}

static u8 fmtChar(Parser * parser, SV char_){

    if(char_.len > 2){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, parser->curr, "Invalid char: %s", CSTR(char_));
        return 0;
    }

    if(char_.data[0] == '\\'){
        char byte = char_.data[1];
        switch(byte){
            case '0': return 0;
            case 'a': return 7;
            case 'b': return 8;
            case 't': return 9;
            case 'n': return 10;
            case 'r': return 13;
            case '\'': return '\'';
            case '\"': return '\"';
            case '\\': return '\\';
            default: break;
        }
    }else{
        return (u8)char_.data[0];
    }
    return 1;
}

static void checkLabel(Parser * parser){

    Token org = parser->tda.items[parser->curr_idx-1];
    Token t = peek(parser);

    Label * ls = (Label*)hashmap_get(parser->jmp_labels, &(Label){.key = t.value});

    if(match(parser, TT_COLON)){

        if(ls){
            log_(SYNTAX_ERROR, &parser->curr.loc, "Cannot redefine a label : '%s'", CSTR(t.value));

            Label * lab = (Label*) hashmap_get(parser->jmp_labels, &(Label){.key = t.value});

            OPCODE first = parser->code_chunk_da.items[lab->value-1];

            PANIC(SYNTAX_ERROR, &first.loc, "First defined here");

        }
        if(parser->conf.is_dgb){
            ByteDA bda = {0};
            for(size_t i = 0; i < t.value.len; i++){
                da_push(bda, t.value.data[i]);
            }
            da_push(parser->code_chunk_da, opcode(OP_NOP, (As) {.bda = bda}, BYTE_ARRAY_, org));
        }else {
            da_push(parser->code_chunk_da, opcode(OP_NOP, (As) {.one = 0}, NONE_, org));
        }
        hashmap_set(parser->jmp_labels, &(Label){.key = t.value, .value = parser->code_addr});//parser->code_chunk_da.count});
        checkIfEntryPoint(parser, t.value);
        code_advance(parser);
    }else if(!ls){
        if(t.type == TT_EOF){
            PANIC_EXPAND_TOK(GENERIC_ERROR, parser->tda.items[parser->tda.count-1], "Unknown identifier : '%s'.", CSTR(parser->tda.items[parser->tda.count-1].value));
            return;
        }else {
            PANIC_EXPAND_TOK(GENERIC_ERROR, t, "Unknown identifier : '%s'.", CSTR(t.value));
            return;
        }
    }

}

static void jmp(Parser * parser, int type){

    Token curr = parser->curr;
    Token t = advance(parser);

    if(t.type != TT_IDENTIFIER && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected label or address after jump opcode. Found: '%s'", CSTR(t.value));
        return;
    }

    if(t.type == TT_IDENTIFIER) {
        Label req_label = {
                .key = t.value,
                .value = parser->code_chunk_da.count
        };
        da_push(parser->req_labels_da, req_label);

        code_advance(parser);
        da_push(parser->code_chunk_da, opcode(type, non, SHORT_, curr));
    }else{
        short addr = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
        code_advance(parser);
        da_push(parser->code_chunk_da, opcode(type, (As){.two = addr}, SHORT_, curr));
    }
}

static void setEntry(Parser * parser){

    Token curr = peek(parser);
    Token t = advance(parser);

    if(t.type != TT_IDENTIFIER && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected label or address as entry point. Found: '%s'", CSTR(t.value));
        return;
    }

    if(parser->entry_point_key.data != NULL) {
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Entry point was already set.");
        return;
    }

    if(t.type == TT_IDENTIFIER){
        SV ident = t.value;

        Label * l = (Label*) hashmap_get(parser->jmp_labels, &(Label){.key = ident});

        if(l){
            parser->is_entry_point_set = true;
            parser->entry = l->value;
        }else{
            parser->entry_point_key = ident;
            parser->is_entry_point_set = false;
        }
    }else{
        short addr = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
        parser->is_entry_point_set = true;
        parser->entry = addr;
    }

}

static void moveTo(Parser * parser){

    Token curr = peek(parser);
    Token t = advance(parser);

    if( t.type != TT_R1 && t.type != TT_R2 && t.type != TT_R3 && t.type != TT_R4 &&
        t.type != TT_R5 && t.type != TT_R6 && t.type != TT_R7 && t.type != TT_R8  &&
        t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected Register or Register Address type after Move operation. Found: '%s'", CSTR(t.value));
        return;
    }

    u8 val = 0;

    if(t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){

        switch(t.type) {
            case TT_R1:
                val = 0;
                break;
            case TT_R2:
                val = 1;
                break;
            case TT_R3:
                val = 2;
                break;
            case TT_R4:
                val = 3;
                break;
            case TT_R5:
                val = 4;
                break;
            case TT_R6:
                val = 5;
                break;
            case TT_R7:
                val = 6;
                break;
            case TT_R8:
                val = 7;
                break;
            default: break;
        }
    }else{
        val = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
    }
    code_advance(parser);
    da_push(parser->code_chunk_da, opcode(OP_MOVE, (As){.one = val}, BYTE_, curr));

}

static void cleanReg(Parser * parser){

    Token curr = peek(parser);
    Token t = advance(parser);

    if( t.type != TT_R1 && t.type != TT_R2 && t.type != TT_R3 && t.type != TT_R4 &&
        t.type != TT_R5 && t.type != TT_R6 && t.type != TT_R7 && t.type != TT_R8 &&
        t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected Register or Register Address type after Get operation. Found: '%s'", CSTR(t.value));
        return;
    }

    u8 val = 0;

    if(t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){

        switch(t.type) {
            case TT_R1:
                val = 0;
                break;
            case TT_R2:
                val = 1;
                break;
            case TT_R3:
                val = 2;
                break;
            case TT_R4:
                val = 3;
                break;
            case TT_R5:
                val = 4;
                break;
            case TT_R6:
                val = 5;
                break;
            case TT_R7:
                val = 6;
                break;
            case TT_R8:
                val = 7;
                break;
            default: break;
        }
    }else{
        val = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
    }
    code_advance(parser);
    da_push(parser->code_chunk_da, opcode(OP_CLEAN, (As){.one = val}, BYTE_, curr));

}

static void getFrom(Parser * parser){

    Token curr = peek(parser);
    Token t = advance(parser);

    if( t.type != TT_R1 && t.type != TT_R2 && t.type != TT_R3 && t.type != TT_R4 &&
        t.type != TT_R5 && t.type != TT_R6 && t.type != TT_R7 && t.type != TT_R8&&
        t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected Register or Register Address type after Get operation. Found: '%s'", CSTR(t.value));
        return;
    }

    u8 val = 0;

    if(t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){

        switch(t.type) {
            case TT_R1:
                val = 0;
                break;
            case TT_R2:
                val = 1;
                break;
            case TT_R3:
                val = 2;
                break;
            case TT_R4:
                val = 3;
                break;
            case TT_R5:
                val = 4;
                break;
            case TT_R6:
                val = 5;
                break;
            case TT_R7:
                val = 6;
                break;
            case TT_R8:
                val = 7;
                break;
            default: break;
        }
    }else{
        val = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
    }
    code_advance(parser);
    da_push(parser->code_chunk_da, opcode(OP_GET, (As){.one = val}, BYTE_, curr));

}

static void castTo(Parser * parser){

    Token curr = parser->curr;

    Token t = advance(parser);

    if( t.type != TT_CHAR &&
        t.type != TT_MEM_PTR &&
        t.type != TT_INTEGER &&
        t.type != TT_DOUBLE &&
        t.type != TT_PTR &&
        t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected Type (CHAR, INPTR, INTEGER, DOUBLE, PTR) after cast opcode. Found: '%s'",
              CSTR(t.value));
        return;
    }

    u8 castType = 0;
    if(t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        switch(t.type){
            case TT_CHAR: castType = 0; break;
            case TT_MEM_PTR: castType = 1; break;
            case TT_INTEGER: castType = 2; break;
            case TT_DOUBLE: castType = 3; break;
            case TT_PTR: castType = 4; break;
            default: break;
        }
    }else{
        castType = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
    }
    code_advance(parser);
    da_push(parser->code_chunk_da, opcode(OP_CAST, (As){.one = castType}, BYTE_, curr));

}

static void push(Parser * parser, int type){

    Token curr = parser->curr;
    Token t = advance(parser);

    switch(type){
        case 0:{
            if(t.type != TT_CHAR_TYPE &&
               t.type != TT_INTEGER_TYPE &&
               t.type != TT_HEX_TYPE &&
               t.type != TT_DOUBLE_TYPE &&
               t.type != TT_STRING &&
               t.type != TT_DOLLAR &&
               t.type != TT_IDENTIFIER){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected value (Char, Integer, Double, String, Pointer Reference, Pointer) after push. Found: '%s'", CSTR(t.value));
                return;
            }

            int ext = strtol(CSTR(t.value), NULL, 10);
            double dext = strtod(CSTR(t.value), NULL);

            switch(t.type){
                case TT_CHAR_TYPE:
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_C, (As) {.one = fmtChar(parser, t.value)}, CHAR_, curr));
                    break;
                case TT_HEX_TYPE:
                    ext = strtol(CSTR(t.value), NULL, 0);
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_I, (As){.four = (s32)ext}, INTEGER_, curr));
                    break;
                case TT_INTEGER_TYPE:
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_I, (As){.four = (s32)ext}, INTEGER_, curr));
                    break;
                case TT_DOUBLE_TYPE:
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_D, (As){.eight = dext}, DOUBLE_, curr));
                    break;
                case TT_STRING: {
                    code_advance(parser);

                    ByteDA bda = {0};

                    for (size_t i = 0; i < t.value.len; i++) {
                        da_push(bda, (u8) t.value.data[i]);
                    }

                    da_push(bda, (u8)0);

                    ByteDA fmt = formatString(parser, bda);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_STR, (As){.bda = fmt}, BYTE_ARRAY_, curr));
                }break;
                case TT_DOLLAR: {
                    t = advance(parser);

                    if (t.type != TT_IDENTIFIER) {
                        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr,
                              "Expected Identifier after Pointer Reference Operator ($). Found: '%s'", CSTR(t.value));
                        return;
                    }

                    SV ident = t.value;

                    MemoryPtr *mem_ptr = (MemoryPtr *) hashmap_get(parser->memory_locations,
                                                                   &(MemoryPtr) {.key = ident});

                    if (!mem_ptr) {
                        PANIC(GENERIC_ERROR, &t.loc, "Unknown identifier: %s", CSTR(ident));
                        return;
                    }

                    size_t ptr = mem_ptr->value;
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_PR, (As) {.two = ptr}, SHORT_, curr));
                }break;
                case TT_IDENTIFIER:{
                    SV ident = t.value;

                    MemoryPtr * mem_ptr = (MemoryPtr*) hashmap_get(parser->memory_locations, &(MemoryPtr){.key = ident});

                    if(!mem_ptr){
                        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Unknown identifier: %s", CSTR(ident));
                        return;
                    }

                    size_t ptr = mem_ptr->value;
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_PUSH_P, (As){.two = ptr}, SHORT_, curr));
                }break;
                default: break;
            }
        }break;
        case TT_PUSH_CHAR:{
            if(t.type != TT_CHAR_TYPE && t.type != TT_INTEGER_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected char after push char. Found: '%s'", CSTR(t.value));
                return;
            }
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_PUSH_C, (As){.one = fmtChar(parser, t.value)}, CHAR_, curr));
        }break;
        case TT_PUSH_INTEGER:{
            if(t.type != TT_INTEGER_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected int after push integer. Found: '%s'", CSTR(t.value));
                return;
            }

            int ext = strtol(CSTR(t.value), NULL, 10);
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_PUSH_I, (As){.four = ext}, INTEGER_, curr));
        }break;
        case TT_PUSH_DOUBLE:{
            if(t.type != TT_DOUBLE_TYPE && t.type != TT_INTEGER_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected double after push double. Found: '%s'", CSTR(t.value));
                return;
            }
            code_advance(parser);
            double dext = strtod(CSTR(t.value), NULL);
            da_push(parser->code_chunk_da, opcode(OP_PUSH_D, (As){.eight = dext}, DOUBLE_, curr));
        }break;
        case TT_PUSH_PTR_REF: {
            if (t.type != TT_IDENTIFIER && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE) {
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr,
                                 "Expected Identifier or Direct Static Address after Pointer Reference Operator ($). Found: '%s'", CSTR(t.value));
                return;
            }

            size_t ptr = 0;
            if(t.type == TT_IDENTIFIER){
                SV ident = t.value;

                MemoryPtr *mem_ptr = (MemoryPtr *) hashmap_get(parser->memory_locations,
                                                               &(MemoryPtr) {.key = ident});

                if (!mem_ptr) {
                    PANIC(GENERIC_ERROR, &t.loc, "Unknown identifier: %s", CSTR(ident));
                    return;
                }

                ptr = mem_ptr->value;
            }else{
                ptr = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
            }
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_PUSH_PR, (As) {.two = ptr}, SHORT_, curr));
        }break;
        case TT_PUSH_IN_PTR: {
            if (t.type != TT_IDENTIFIER && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE) {
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr,
                                 "Expected Identifier or Direct Static Address after push static pointer. Found: '%s'", CSTR(t.value));
                return;
            }

            size_t ptr = 0;
            if(t.type == TT_IDENTIFIER){
                SV ident = t.value;

                MemoryPtr *mem_ptr = (MemoryPtr *) hashmap_get(parser->memory_locations,
                                                               &(MemoryPtr) {.key = ident});

                if (!mem_ptr) {
                    PANIC(GENERIC_ERROR, &t.loc, "Unknown identifier: %s", CSTR(ident));
                    return;
                }

                ptr = mem_ptr->value;
            }else{
                ptr = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
            }
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_PUSH_P, (As) {.two = ptr}, SHORT_, curr));
        }break;
        case TT_PUSH_STRING:{
            if(t.type != TT_STRING && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected String or list of bytes after push string. Found: '%s'", CSTR(t.value));
                return;
            }

            ByteDA bda = {0};
            size_t len = 0;

            switch(t.type){
                case TT_STRING: {

                    for(size_t i = 0; i < t.value.len; i++){
                        da_push(bda, (u8)t.value.data[i]);
                    }

                    len += t.value.len;
                }break;
                case TT_INTEGER_TYPE: {

                    int val = strtol(CSTR(t.value), 0, 10);

                    da_push(bda, (u8)val);

                    len += 1;
                }break;
                case TT_HEX_TYPE: {

                    int val = strtol(CSTR(t.value), 0, 0);

                    da_push(bda, (u8)val);

                    len += 1;
                }break;
                default: break;
            }

            if(match(parser, TT_COMMA)) {
                while(1) {

                    t = advance(parser);

                    if (t.type != TT_STRING && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE) {
                        PANIC_EXPAND_TOK(SYNTAX_ERROR, t,
                                         "Expected String literal or byte after identifier in String definition. Found: '%s'",
                                         CSTR(t.value));
                        return;
                    }

                    switch (t.type) {
                        case TT_STRING: {

                            for (size_t i = 0; i < t.value.len; i++) {
                                da_push(bda, (u8) t.value.data[i]);
                            }

                            len += t.value.len;
                        }break;
                        case TT_INTEGER_TYPE: {

                            int val = strtol(CSTR(t.value), 0, 10);

                            da_push(bda, (u8) val);

                            len += 1;
                        }break;
                        case TT_HEX_TYPE: {

                            int val = strtol(CSTR(t.value), 0, 0);

                            da_push(bda, (u8)val);

                            len += 1;
                        }break;
                        default: break;
                    }

                    if (!match(parser, TT_COMMA)) {
                        break;
                    }

                }
            }
            ByteDA fmt = formatString(parser, bda);
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_PUSH_STR, (As){.bda = fmt}, BYTE_ARRAY_, curr));
        }break;
        default: break;
    }

}

static void dup(Parser * parser){

    Token curr = parser->curr;
    Token t = advance(parser);

    if(t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected distance value (Integer) after dup. Found: '%s'", CSTR(t.value));
        return;
    }

    int ext = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
    code_advance(parser);
    da_push(parser->code_chunk_da, opcode(OP_DUP, (As){.two = (s16)ext}, SHORT_, curr));

}

static void compare(Parser * parser, int cmp_type){

    Token curr = parser->curr;
    Token t = advance(parser);

    switch(cmp_type){
        case 0:{
            if(t.type != TT_CHAR_TYPE &&
               t.type != TT_INTEGER_TYPE &&
               t.type != TT_HEX_TYPE &&
               t.type != TT_DOUBLE_TYPE &&
               t.type != TT_DOLLAR){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected value (Byte, Char, Short, Integer, Double, PtrReference) after push. Found: '%s'", CSTR(t.value));
                return;
            }

            switch(t.type){
                case TT_CHAR_TYPE: {
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_CMP_C, (As) {.one = fmtChar(parser, t.value)}, CHAR_, curr));
                }break;
                case TT_HEX_TYPE: {
                    int ext = strtol(CSTR(t.value), NULL, 0);
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_CMP_I, (As) {.four = (s32) ext}, INTEGER_, curr));
                }break;
                case TT_INTEGER_TYPE: {
                    int ext = strtol(CSTR(t.value), NULL, 10);
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_CMP_I, (As) {.four = (s32) ext}, INTEGER_, curr));
                }break;
                case TT_DOUBLE_TYPE: {
                    double dext = strtod(CSTR(t.value), NULL);
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_CMP_D, (As) {.eight = dext}, DOUBLE_, curr));
                }break;
                case TT_DOLLAR: {

                    t = advance(parser);

                    if(t.type != TT_IDENTIFIER){
                        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected Identifier after Pointer Reference Operator ($). Found: '%s'", CSTR(t.value));
                        return;
                    }

                    SV ident = t.value;

                    MemoryPtr * mem_ptr = (MemoryPtr*) hashmap_get(parser->memory_locations, &(MemoryPtr){.key = ident});

                    size_t ptr = mem_ptr->value;
                    code_advance(parser);
                    da_push(parser->code_chunk_da, opcode(OP_CMP_PR, (As){.two = ptr}, SHORT_, curr));

                }
                default: break;
            }
        }break;
        case TT_COMPARE_CHAR:{
            if(t.type != TT_CHAR_TYPE &&
               t.type != TT_INTEGER_TYPE &&
               t.type != TT_HEX_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected value (Char, Integer) after compare char. Found: '%s'", CSTR(t.value));
                return;
            }
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_CMP_C, (As) {.one = fmtChar(parser, t.value)}, CHAR_, curr));
        }break;
        case TT_COMPARE_INTEGER:{
            if(t.type != TT_INTEGER_TYPE &&
               t.type != TT_HEX_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected value (Integer) after compare integer. Found: '%s'", CSTR(t.value));
                return;
            }
            int ext = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_CMP_I, (As) {.four = (s32) ext}, INTEGER_, curr));
        }break;
        case TT_COMPARE_DOUBLE:{
            if(t.type != TT_DOUBLE_TYPE && t.type != TT_INTEGER_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected value (Double) after compare double. Found: '%s'", CSTR(t.value));
                return;
            }
            double dext = strtod(CSTR(t.value), NULL);
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_CMP_D, (As) {.eight = dext}, DOUBLE_, curr));
        }break;
        case TT_COMPARE_PTR_REF: {

            if(t.type != TT_IDENTIFIER && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
                PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected Identifier or Static Address after Pointer Reference Operator ($). Found: '%s'", CSTR(t.value));
                return;
            }

            size_t ptr = 0;
            if(t.type == TT_IDENTIFIER){
                SV ident = t.value;

                MemoryPtr * mem_ptr = (MemoryPtr*) hashmap_get(parser->memory_locations, &(MemoryPtr){.key = ident});

                ptr = mem_ptr->value;
            }else{
                ptr = strtol(CSTR(t.value), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);
                code_advance(parser);
            }
            code_advance(parser);
            da_push(parser->code_chunk_da, opcode(OP_CMP_PR, (As){.two = ptr}, SHORT_, curr));

        }break;
    }

}

static void const_def(Parser * parser){

    Token curr = parser->curr;
    Token t = advance(parser);

    if(t.type != TT_IDENTIFIER){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected identifier after String definition. Found: '%s'", CSTR(t.value));
        return;
    }

    Token id_t = t;
    SV ident = id_t.value;

    IdentTok * id = (IdentTok* ) hashmap_get(parser->identifiers, &(IdentTok){.key = ident});

    if(id){
        log_(GENERIC_ERROR, &t.loc, "Cannot redefine existing Name as a constant.");
        PANIC_EXPAND_TOK(GENERIC_ERROR, id->tok, "First defined here");
        return;
    }

    t = advance(parser);

    if( t.type != TT_CHAR_DEF &&
        t.type != TT_INTEGER_DEF &&
        t.type != TT_DOUBLE_DEF){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected Type of constant (DC, DI, DD) after identifier in constant definition. Found: '%s'", CSTR(t.value));
        return;
    }

    _TokenType tok_type = t.type;

    t = advance(parser);

    if( t.type != TT_CHAR_TYPE &&
        t.type != TT_INTEGER_TYPE &&
        t.type != TT_HEX_TYPE &&
        t.type != TT_DOUBLE_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected Constant value (Char, Integer, Double) after constant type in constant definition. Found: '%s'", CSTR(t.value));
        return;
    }

    SV sv_val = t.value;

    As val;
    size_t len = 0;
    int ext = strtol(CSTR(sv_val), NULL, t.type == TT_HEX_TYPE ? 0 : 10);

    switch(tok_type){
        case TT_CHAR_DEF:
            val.one = fmtChar(parser, t.value);
            len += 1;

            da_push(parser->data_chunk_da, opcode(OP_CONST_CHAR_DEF, val, CHAR_, curr));
            break;
        case TT_INTEGER_DEF:
            val.four = (s32)ext;
            len += 4;

            da_push(parser->data_chunk_da, opcode(OP_CONST_INTEGER_DEF, val, INTEGER_, curr));
            break;
        case TT_DOUBLE_DEF: {
            double dext = strtod(CSTR(sv_val), NULL);
            val.eight = dext;
            len += 8;

            da_push(parser->data_chunk_da, opcode(OP_CONST_DOUBLE_DEF, val, DOUBLE_, curr));
        }break;
        default: break;
    }

    hashmap_set(parser->memory_locations, &(MemoryPtr){.key = ident, .value = parser->curr_ptr});

    parser->curr_ptr += len;

    hashmap_set(parser->identifiers, &(IdentTok){.key = ident, .tok = id_t});

}

static void string_def(Parser * parser){

    Token curr = parser->curr;
    Token t = advance(parser);

    if(t.type != TT_IDENTIFIER){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected identifier after String definition. Found: '%s'", CSTR(t.value));
        return;
    }

    Token id_t = t;
    SV ident = id_t.value;

    IdentTok * id = (IdentTok* ) hashmap_get(parser->identifiers, &(IdentTok){.key = ident});

    if(id){
        log_(GENERIC_ERROR, &t.loc, "Cannot redefine existing Name as a string.");
        PANIC_EXPAND_TOK(GENERIC_ERROR, id->tok, "First defined here");
        return;
    }

    t = advance(parser);

    if(t.type != TT_STRING && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected String literal or byte after identifier in String definition. Found: '%s'", CSTR(t.value));
        return;
    }

    size_t len = 0;
    ByteDA bda = {0};

    switch(t.type){
        case TT_STRING: {

            for(size_t i = 0; i < t.value.len; i++){
                da_push(bda, (u8)t.value.data[i]);
            }

            len += t.value.len;
        }break;
        case TT_INTEGER_TYPE: {

            int val = strtol(CSTR(t.value), 0, 10);

            da_push(bda, (u8)val);

            len += 1;
        }break;
        case TT_HEX_TYPE: {

            int val = strtol(CSTR(t.value), 0, 0);

            da_push(bda, (u8)val);

            len += 1;
        }break;
        default: break;
    }

    if(match(parser, TT_COMMA)) {
        while(1) {

            t = advance(parser);

            if (t.type != TT_STRING && t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE) {
                PANIC_EXPAND_TOK(SYNTAX_ERROR, t,
                      "Expected String literal or byte after identifier in String definition. Found: '%s'",
                      CSTR(t.value));
                return;
            }

            switch (t.type) {
                case TT_STRING: {

                    for (size_t i = 0; i < t.value.len; i++) {
                        da_push(bda, (u8) t.value.data[i]);
                    }

                    len += t.value.len;
                }break;
                case TT_INTEGER_TYPE: {

                    int val = strtol(CSTR(t.value), 0, 10);

                    da_push(bda, (u8) val);

                    len += 1;
                }break;
                case TT_HEX_TYPE: {

                    int val = strtol(CSTR(t.value), 0, 0);

                    da_push(bda, (u8)val);

                    len += 1;
                }break;
                default: break;
            }

            if (!match(parser, TT_COMMA)) {
                break;
            }

        }
    }

    hashmap_set(parser->memory_locations, &(MemoryPtr){.key = ident, .value = parser->curr_ptr});
    hashmap_set(parser->identifiers, &(IdentTok){.key = ident, .tok = id_t});
    emitString(parser, bda, curr);
}

static void mem_def(Parser * parser){

    Token curr = parser->curr;
    Token t = advance(parser);

    if(t.type != TT_IDENTIFIER){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, curr, "Expected identifier after String definition. Found: '%s'", CSTR(t.value));

        return;}

    Token id_t = t;
    SV ident = id_t.value;

    t = advance(parser);

    if( t.type != TT_INTEGER_TYPE && t.type != TT_HEX_TYPE){
        PANIC_EXPAND_TOK(SYNTAX_ERROR, t, "Expected Integer after memory zone identifier in memory zone definition. Found: '%s'", CSTR(t.value));
        return;
    }

    SV sv_val = t.value;
    int ext = strtol(CSTR(sv_val), NULL, t.type == TT_INTEGER_TYPE ? 10 : 0);

    da_push(parser->data_chunk_da, opcode(OP_MEM_DEF, (As){.four = ext}, INTEGER_, curr));

    hashmap_set(parser->memory_locations, &(MemoryPtr){.key = ident, .value = parser->curr_ptr});

    parser->curr_ptr += (size_t)ext;

    hashmap_set(parser->identifiers, &(IdentTok){.key = ident, .tok = id_t});
}

static void skip_macro(Parser * parser){

    while(!match(parser, TT_MACRO_END)) {advance(parser);}

}

#include <sys/time.h>

static int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

int parse(Parser * parser){

    struct timespec start, end;

    if(parser->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    while(!isAtEnd(parser)){
        parser->curr = advance(parser);
        switch(parser->curr.type){
            case TT_IDENTIFIER:
                checkLabel(parser);
                break;
            case TT_ENTRY:
                if(parser->conf.is_dgb){
                    parser->curr_addr -= 5;
                }
                parser->curr_addr -= 2;
                setEntry(parser);
                break;
            case TT_MOVE:
                moveTo(parser);
                break;
            case TT_GET:
                getFrom(parser);
                break;
            case TT_CLEAN:
                cleanReg(parser);
                break;
            case TT_DEREF:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_DEREF, non, NONE_, parser->curr));
                break;
            case TT_CAST:
                castTo(parser);
                break;
            case TT_PUSH:
                push(parser, 0);
                break;
            case TT_PUSH_CHAR:
                push(parser, TT_PUSH_CHAR);
                break;
            case TT_PUSH_INTEGER:
                push(parser, TT_PUSH_INTEGER);
                break;
            case TT_PUSH_DOUBLE:
                push(parser, TT_PUSH_DOUBLE);
                break;
            case TT_PUSH_IN_PTR:
                push(parser, TT_PUSH_IN_PTR);
                break;
            case TT_PUSH_PTR_REF:
                push(parser, TT_PUSH_PTR_REF);
                break;
            case TT_PUSH_STRING:
                push(parser, TT_PUSH_STRING);
                break;
            case TT_ADD_CHAR:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_ADD_C, non, NONE_, parser->curr));
                break;
            case TT_SUB_CHAR:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_SUB_C, non, NONE_, parser->curr));
                break;
            case TT_MUL_CHAR:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_MUL_C, non, NONE_, parser->curr));
                break;
            case TT_DIV_CHAR:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_DIV_C,non, NONE_, parser->curr));
                break;
            case TT_ADD_INT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_ADD_I, non, NONE_, parser->curr));
                break;
            case TT_SUB_INT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_SUB_I, non, NONE_, parser->curr));
                break;
            case TT_MUL_INT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_MUL_I, non, NONE_, parser->curr));
                break;
            case TT_DIV_INT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_DIV_I,non, NONE_, parser->curr));
                break;
            case TT_ADD_DOUBLE:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_ADD_D, non, NONE_, parser->curr));
                break;
            case TT_SUB_DOUBLE:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_SUB_D, non, NONE_, parser->curr));
                break;
            case TT_MUL_DOUBLE:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_MUL_D, non, NONE_, parser->curr));
                break;
            case TT_DIV_DOUBLE:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_DIV_D,non, NONE_, parser->curr));
                break;
            case TT_DUMP:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_DUMP, non, NONE_, parser->curr));
                break;
            case TT_EXIT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_EXIT, non, NONE_, parser->curr));
                break;
            case TT_DUP:
                dup(parser);
                break;
            case TT_JMP:
                jmp(parser, OP_JMP);
                break;
            case TT_JMP_ZERO:
                jmp(parser, OP_JMP_Z);
                break;
            case TT_JMP_NOT_ZERO:
                jmp(parser, OP_JMP_NZ);
                break;
            case TT_JMP_GREATER:
                jmp(parser, OP_JMP_G);
                break;
            case TT_JMP_GREATER_EQUAL:
                jmp(parser, OP_JMP_GE);
                break;
            case TT_JMP_LESS:
                jmp(parser, OP_JMP_L);
                break;
            case TT_JMP_LESS_EQUAL:
                jmp(parser, OP_JMP_LE);
                break;
            case TT_JMP_EQUAL:
                jmp(parser, OP_JMP_E);
                break;
            case TT_JMP_NOT_EQUAL:
                jmp(parser, OP_JMP_NE);
                break;
            case TT_CONST_DEF:
                const_def(parser);
                break;
            case TT_STRING_DEF:
                string_def(parser);
                break;
            case TT_MEMORY:
                mem_def(parser);
                break;
            case TT_COMPARE:
                compare(parser,0);
                break;
            case TT_COMPARE_CHAR:
                compare(parser, TT_COMPARE_CHAR);
                break;
            case TT_COMPARE_INTEGER:
                compare(parser, TT_COMPARE_INTEGER);
                break;
            case TT_COMPARE_DOUBLE:
                compare(parser, TT_COMPARE_DOUBLE);
                break;
            case TT_COMPARE_PTR_REF:
                compare(parser, TT_COMPARE_PTR_REF);
                break;
            case TT_STORE:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_STORE, non, NONE_, parser->curr));
                break;
            case TT_DROP:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_DROP, non, NONE_, parser->curr));
                break;
            case TT_CALL:
                jmp(parser, OP_CALL);
                break;
            case TT_RET:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_RET, non, NONE_, parser->curr));
                break;
            case TT_INCLUDE:
                break;
            case TT_MACRO_DEF:
                skip_macro(parser);
                break;
            case TT_END_INCLUDE:
                if(!parser->is_entry_point_set){
                    parser->entry = parser->code_addr;
                }
                break;
            case TT_END:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_END, non, NONE_, parser->curr));
                break;
                break;
            case TT_SHIFT_LEFT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_SHL, non, NONE_, parser->curr));
                break;
            case TT_SHIFT_RIGHT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_SHR, non, NONE_, parser->curr));
                break;
            case TT_AND:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_AND, non, NONE_, parser->curr));
                break;
            case TT_OR:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_OR, non, NONE_, parser->curr));
                break;
            case TT_NOT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_NOT, non, NONE_, parser->curr));
                break;
            case TT_STACK_PRINT:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_STACKPRINT, non, NONE_, parser->curr));
                break;
            case TT_SWAP:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_SWAP, non, NONE_, parser->curr));
                break;
            case TT_INTERNAL:
                code_advance(parser);
                da_push(parser->code_chunk_da, opcode(OP_INTERNAL, non, NONE_, parser->curr));
                break;
            default:
                PANIC_EXPAND_TOK(GENERIC_ERROR, parser->curr, "Unknown token: '%s'", CSTR(peek(parser).value));
        }
        if(parser->err_code == -1){
            return parser->err_code;
        }
    }

    for(size_t i = 0; i < parser->req_labels_da.count; i++){
        SV ident = parser->req_labels_da.items[i].key;
        size_t idx = parser->req_labels_da.items[i].value;
        Label * label = (Label*) hashmap_get(parser->jmp_labels, &(Label){.key = ident});
        if(!label){
            PANIC(GENERIC_ERROR, &parser->code_chunk_da.items[idx].loc, "Unknown label for the jump: %s", CSTR(ident));
        }
        parser->code_chunk_da.items[idx].as.two = label->value;
    }

    if(parser->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Parsing took %lf seconds.",
             (timespecDiff(&end, &start) * 0.000000001));
    }

    free(parser->tda.items);
    free(parser->req_labels_da.items);

    hashmap_free(parser->jmp_labels);
    hashmap_free(parser->identifiers);
    hashmap_free(parser->memory_locations);

    return parser->err_code;
}
