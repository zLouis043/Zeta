#include "Lexer.h"

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include "common/DyArray.h"
#include "common/Hashmap.h"
#include "common/Log.h"

struct keyword {
    SV key;
    _TokenType type;
};

struct hashmap *m;

void set_loc(Location *loc, SV filepath, SV lineSV, size_t line, size_t column){
    loc->filepath = filepath;
    loc->lineSV = lineSV;
    loc->column = column;
    loc->line = line;
}

// Just for laziness (I changed from a custom hashmap to a better one)
#define map_add_el(map, sv, t) hashmap_set(map, &(struct keyword){ .key = sv, .type = t});

void set_up_keywords(){

    map_add_el(m, SV("end"), TT_END);

    map_add_el(m, SV("DC"), TT_CHAR_DEF);
    map_add_el(m, SV("DI"), TT_INTEGER_DEF);
    map_add_el(m, SV("DD"), TT_DOUBLE_DEF);

    map_add_el(m, SV("CHAR"), TT_CHAR);
    map_add_el(m, SV("PTR"), TT_PTR);
    map_add_el(m, SV("INPTR"), TT_MEM_PTR);
    map_add_el(m, SV("INTEGER"), TT_INTEGER);
    map_add_el(m, SV("DOUBLE"), TT_DOUBLE);

    map_add_el(m, SV("INCLUDE"), TT_INCLUDE);
    map_add_el(m, SV("DATA_START"), TT_DATA_START);
    map_add_el(m, SV("DATA_END"), TT_DATA_END);
    map_add_el(m, SV("CODE_START"), TT_CODE_START);
    map_add_el(m, SV("CODE_END"), TT_CODE_END);
    map_add_el(m, SV("EXPORT_START"), TT_EXPORT_START);
    map_add_el(m, SV("EXPORT_END"), TT_EXPORT_END);
    map_add_el(m, SV("ENTRY"), TT_ENTRY);

    map_add_el(m, SV("move"), TT_MOVE);
    map_add_el(m, SV("get"), TT_GET);
    map_add_el(m, SV("deref"), TT_DEREF);
    map_add_el(m, SV("cast"), TT_CAST);

    map_add_el(m, SV("push"), TT_PUSH);
    map_add_el(m, SV("pushC"), TT_PUSH_CHAR);
    map_add_el(m, SV("pushI"), TT_PUSH_INTEGER);
    map_add_el(m, SV("pushD"), TT_PUSH_DOUBLE);
    map_add_el(m, SV("pushP"), TT_PUSH_IN_PTR);
    map_add_el(m, SV("pushPR"), TT_PUSH_PTR_REF);
    map_add_el(m, SV("pushSTR"), TT_PUSH_STRING);

    map_add_el(m, SV("addC"), TT_ADD_CHAR);
    map_add_el(m, SV("subC"), TT_SUB_CHAR);
    map_add_el(m, SV("mulC"), TT_MUL_CHAR);
    map_add_el(m, SV("divC"), TT_DIV_CHAR);

    map_add_el(m, SV("addI"), TT_ADD_INT);
    map_add_el(m, SV("subI"), TT_SUB_INT);
    map_add_el(m, SV("mulI"), TT_MUL_INT);
    map_add_el(m, SV("divI"), TT_DIV_INT);

    map_add_el(m, SV("addD"), TT_ADD_DOUBLE);
    map_add_el(m, SV("subD"), TT_SUB_DOUBLE);
    map_add_el(m, SV("mulD"), TT_MUL_DOUBLE);
    map_add_el(m, SV("divD"), TT_DIV_DOUBLE);

    map_add_el(m, SV("STRING"), TT_STRING_DEF);

    map_add_el(m, SV("exit"), TT_EXIT);
    map_add_el(m, SV("stackState"), TT_STACK_PRINT);

    map_add_el(m, SV("dup"), TT_DUP);

    map_add_el(m, SV("jmp"), TT_JMP);
    map_add_el(m, SV("jmp_z"), TT_JMP_ZERO);
    map_add_el(m, SV("jmp_nz"), TT_JMP_NOT_ZERO);
    map_add_el(m, SV("jmp_ge"), TT_JMP_GREATER_EQUAL);
    map_add_el(m, SV("jmp_g"), TT_JMP_GREATER);
    map_add_el(m, SV("jmp_le"), TT_JMP_LESS_EQUAL);
    map_add_el(m, SV("jmp_l"), TT_JMP_LESS);
    map_add_el(m, SV("jmp_e"), TT_JMP_EQUAL);
    map_add_el(m, SV("jmp_ne"), TT_JMP_NOT_EQUAL);

    map_add_el(m, SV("CONST"), TT_CONST_DEF);

    map_add_el(m, SV("cmp"), TT_COMPARE);
    map_add_el(m, SV("cmpC"), TT_COMPARE_CHAR);
    map_add_el(m, SV("cmpI"), TT_COMPARE_INTEGER);
    map_add_el(m, SV("cmpD"), TT_COMPARE_DOUBLE);
    map_add_el(m, SV("cmpPR"), TT_COMPARE_PTR_REF);
    map_add_el(m, SV("drop"), TT_DROP);
    map_add_el(m, SV("dump"), TT_DUMP);
    map_add_el(m, SV("swap"), TT_SWAP);

    map_add_el(m, SV("call"), TT_CALL);
    map_add_el(m, SV("ret"), TT_RET);

    map_add_el(m, SV("R1"), TT_R1);
    map_add_el(m, SV("R2"), TT_R2);
    map_add_el(m, SV("R3"), TT_R3);
    map_add_el(m, SV("R4"), TT_R4);
    map_add_el(m, SV("R5"), TT_R5);
    map_add_el(m, SV("R6"), TT_R6);
    map_add_el(m, SV("R7"), TT_R7);
    map_add_el(m, SV("R8"), TT_R8);
    map_add_el(m, SV("clean"), TT_CLEAN);

    map_add_el(m, SV("shl"), TT_SHIFT_LEFT);
    map_add_el(m, SV("shr"), TT_SHIFT_RIGHT);
    map_add_el(m, SV("and"), TT_AND);
    map_add_el(m, SV("or"), TT_OR);
    map_add_el(m, SV("not"), TT_NOT);

    map_add_el(m, SV("MEMORY"), TT_MEMORY);
    map_add_el(m, SV("store"), TT_STORE);

    map_add_el(m, SV("MACRO"), TT_MACRO_DEF);
    map_add_el(m, SV("MACRO_END"), TT_MACRO_END);
    map_add_el(m, SV("stack_print"), TT_STACK_PRINT);
    map_add_el(m, SV("internal"), TT_INTERNAL);

}

int keyword_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const struct keyword *ua = a;
    const struct keyword *ub = b;
    return sv_eq(ua->key, ub->key);
}

/*bool keyword_iter(const void *item, void *udata) {
    const struct keyword *kw = item;
    printf("%s (age=%d)\n", CSTR(kw->key), kw->type);
    return true;
}*/

uint64_t keyword_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct keyword *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

void lexer_init(Lexer * lexer, char * filepath){

    SV filepathSV = SV(filepath);

    LinesDA lda = {0};

    if(get_lines_from_file(filepathSV, &lda) < 0){
        log_(LEXING_ERROR, NULL, "Could not get file lines.", CSTR(filepathSV));
        exit(-1);
    }

    lexer->lda = lda;
    lexer->filepath = filepathSV;
    lexer->start = 0;
    set_loc(&lexer->curr, filepathSV, lda.items[0], 0, 0);

    m = hashmap_new(sizeof(struct keyword), 0, 0, 0,
                keyword_hash, keyword_compare, NULL, NULL);
    set_up_keywords();
}

static bool isAtEnd(Lexer * lexer){
    return lexer->curr.column == lexer->lda.items[lexer->curr.line].len;
}

static char advance(Lexer * lexer){

    char c = lexer->lda.items[lexer->curr.line].data[lexer->curr.column];

    lexer->curr.column++;

    return c;

}

static char peek(Lexer * lexer){
    if(isAtEnd(lexer)) return '\0';
    return lexer->lda.items[lexer->curr.line].data[lexer->curr.column];
}

static char peekNext(Lexer * lexer){
    if(lexer->curr.column + 1 > lexer->lda.items[lexer->curr.line].len) return '\0';
    return lexer->lda.items[lexer->curr.line].data[lexer->curr.column+1];
}

static bool match(Lexer * lexer, char expected){
    if(isAtEnd(lexer)) return false;
    if(peek(lexer) != expected) return false;

    lexer->curr.column++;
    return true;
}

bool isNum(char c){
    return (c >= '0' && c <= '9');
}

bool isAlpha(char c){
    return  (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
             c == '_';
}

bool isAlphaNumeric(char c){
    return isAlpha(c) || isNum(c);
}

void err_unknown_token(Lexer * lexer){
    PANIC(LEXING_ERROR, &lexer->curr, "Unknown Token: '%c'", lexer->lda.items[lexer->curr.line].data[lexer->curr.column-1]);
}

void string(Lexer * lexer){

    while(peek(lexer) != '"' && !isAtEnd(lexer)){
        if(peek(lexer) == '\\' && peekNext(lexer) == '"') advance(lexer);
        advance(lexer);
    }

    if(isAtEnd(lexer)){
        PANIC(LEXING_ERROR, &lexer->curr, "Unclosed String Literal");
    }

    advance(lexer);

    SV value = sv_substring(lexer->curr.lineSV, lexer->start+1, lexer->curr.column-1);
    Location loc_new = loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1);
    Token tok = token(value, TT_STRING, loc_new);
    da_push(lexer->tda, tok);
}

void identifier(Lexer * lexer){
    while(isAlphaNumeric(peek(lexer))) advance(lexer);

    SV text = sv_substring(lexer->lda.items[lexer->curr.line], lexer->start, lexer->curr.column);

    Location loc_new = loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1);
    struct keyword * kw = (struct keyword * )(hashmap_get(m, &(struct keyword){.key = text}));

    Token tok;
    _TokenType type;

    if(kw == NULL){
        type = TT_IDENTIFIER;
    }else{
        type = kw->type;
    }

    tok = token(text, type, loc_new);

    da_push(lexer->tda, tok);
}

void number(Lexer * lexer){

        bool is_d = false;

        while(isNum(peek(lexer))){ advance(lexer);}

        if(peek(lexer) == '.' && isNum(peekNext(lexer))){
            is_d = true;
            advance(lexer);

            while(isNum(peek(lexer))) advance(lexer);
        }

        if(peek(lexer) == '.' && is_d){
            PANIC(LEXING_ERROR, &lexer->curr, "Invalid number.");
        }

        SV text = sv_substring(lexer->lda.items[lexer->curr.line], lexer->start, lexer->curr.column);
        Location loc_new = loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1);
        Token tok;
        if(is_d){
            tok = token(text, TT_DOUBLE_TYPE, loc_new);
        }else{

            _TokenType type = TT_INTEGER_TYPE;

            tok = token(text, type, loc_new);

        }

    da_push(lexer->tda, tok);
}

void _char(Lexer * lexer){

    advance(lexer);

    while(peek(lexer) != '\'' && !isAtEnd(lexer)){
        advance(lexer);
    }

    if(isAtEnd(lexer)){
        PANIC(LEXING_ERROR, &lexer->curr, "Unclosed Char");
    }

    SV char_  = sv_substring(lexer->lda.items[lexer->curr.line], lexer->start+1, lexer->curr.column);

    advance(lexer);
    Location loc_new = loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1);
    Token tok = token(char_, TT_CHAR_TYPE, loc_new);
    da_push(lexer->tda, tok);

}

void hex(Lexer * lexer){

    if(match(lexer, 'x') || match(lexer, 'X')){
        advance(lexer);
        while(isAlphaNumeric(peek(lexer))){ advance(lexer);}
        SV hexa = sv_substring(lexer->lda.items[lexer->curr.line], lexer->start, lexer->curr.column);
        Location loc_new = loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1);
        Token tok = token(hexa, TT_HEX_TYPE, loc_new);
        da_push(lexer->tda, tok);
    }else{
        SV zero = sv_substring(lexer->lda.items[lexer->curr.line], lexer->start, lexer->curr.column);
        Location loc_new = loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1);
        Token tok = token(zero, TT_INTEGER_TYPE, loc_new);
        da_push(lexer->tda, tok);
    }

}

void get_token(Lexer * lexer){

    char c = advance(lexer);
    switch(c){
        case ';':{
            if(match(lexer, ';')){
                while(!isAtEnd(lexer)) advance(lexer);
            }
            return;
        }
        case '[':{
            Token tok = token(SV("["), TT_OPEN_SQUARE, loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1));
            da_push(lexer->tda, tok);
            return;
        }
        case ']':{
            Token tok = token(SV("]"), TT_CLOSE_SQUARE, loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1));
            da_push(lexer->tda, tok);
            return;
        }
        case '.':{
            Token tok = token(SV("."), TT_DOT, loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1));
            da_push(lexer->tda, tok);
            return;
        }
        case ',':{
            Token tok = token(SV(","), TT_COMMA, loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1));
            da_push(lexer->tda, tok);
            return;
        }
        case ':':{
            Token tok = token(SV(":"), TT_COLON, loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1));
            da_push(lexer->tda, tok);
            return;
        }
        case '$':{
            Token tok = token(SV("$"), TT_DOLLAR, loc(lexer->curr.filepath, lexer->curr.lineSV, lexer->curr.line, lexer->start+1));
            da_push(lexer->tda, tok);
            return;
        }
        case '\'':{
            _char(lexer);
        }break;
        case ' ':
        case '\t':
        case '\r':
            break;
        case '"':{
            string(lexer);
        }break;
        case '0':{
            hex(lexer);
        }break;
        default:{
            if(isAlpha(c)){
                identifier(lexer);
            }else if(isNum(c) || c == '-'){
                number(lexer);
            }else{
                err_unknown_token(lexer);
            }
        }break;
    }

}

void lex_line(Lexer * lexer){
    while(!isAtEnd(lexer)){
        lexer->start = lexer->curr.column;
        get_token(lexer);
    }
}

static int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

void lex(Lexer * lexer, Configs conf){
    struct timespec start, end;

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    for(size_t i = 0; i < lexer->lda.count; i++){
        set_loc(&lexer->curr, lexer->filepath, lexer->lda.items[i], i, 0);
        lex_line(lexer);
    }

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Lexing file %.*s took %lf seconds.",
             lexer->filepath.len, lexer->filepath.data,
             (timespecDiff(&end, &start) * 0.000000001));
    }

    hashmap_clear(m, false);
    hashmap_free(m);

#if 0
    FILE * fp = fopen("log_lex.txt", "w");

    if(!fp) {
        log(ERROR, NULL, "Could not create log file for lexing.");
    }

    fprintf(stdout, "%2zu |   %-22s  | %-15s | FILE_INFO\n", 0 , "TEXT VALUE", "TOK_TYPE");
    for(int i = 0; i < 83; i++){
        fprintf(stdout, "=");
    }
    fprintf(stdout, "\n");

    for(size_t i = 0; i < lexer->tda.count; i++){\
        fprintf(stdout, "%2zu |  %s\n", i+1, tok_to_string(lexer->tda.items[i]));\
    }

    fclose(fp);
#endif // LEX_DUMP

}

