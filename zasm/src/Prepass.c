#include "Prepass.h"

#include "common/Log.h"

#include <stdio.h>

static int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

int Macro_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const Macro *ua = a;
    const Macro *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t Macro_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Macro *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

int SKW_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const SpecialKeyWord *ua = a;
    const SpecialKeyWord *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t SKW_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const SpecialKeyWord *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

int Include_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const Include *ua = a;
    const Include *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t Include_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const Include *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

#if WIN32
#include <Windows.h>
#endif

#include "common/cwalk.h"

char * get_absolute_path(SV filepath){
#if WIN32
    char * path = malloc(1024);
    _fullpath(path, CSTR(filepath), 1024);
    return path;
#else
#endif // WIN32
}

void prepass_init(Prepass * pp){
    Block zero = {0};
    pp->temp_block = zero;
    pp->lexer_base_set = false;
    pp->curr_macro_depth = 0;
    pp->curr_include_depth = 0;
    pp->is_head_label_set = false;
    pp->macros = hashmap_new(sizeof(Macro), 0, 0, 0,
                    Macro_hash, Macro_compare, NULL, NULL);
    pp->includes = hashmap_new(sizeof(Include), 0, 0, 0,
                               Include_hash, Include_compare, NULL, NULL);
    pp->expanded_includes = hashmap_new(sizeof(Expanded_Include), 0, 0, 0,
                               Include_hash, Include_compare, NULL, NULL);
    pp->special_key_words = hashmap_new(sizeof(SpecialKeyWord), 0, 0, 0,
                                        SKW_hash, SKW_compare, NULL, NULL);
    hashmap_set(pp->special_key_words, &(SpecialKeyWord){.key = SV("CURR_LINE"), .id = 0});
    hashmap_set(pp->special_key_words, &(SpecialKeyWord){.key = SV("CURR_COL"), .id = 1});
    hashmap_set(pp->special_key_words, &(SpecialKeyWord){.key = SV("CURR_FILE"), .id = 2});
}

void get_directives(Prepass * pp, SV filepath, Configs conf) {

    struct timespec start, end;

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    Lexer lexer = {0};

    lexer_init(&lexer, CSTR(filepath));

    lex(&lexer, conf);

    da_push(pp->fda, filepath);

    if (!pp->lexer_base_set) {
        pp->lexer_base = lexer;
        pp->lexer_base_set = true;
    }

    pp->lexer_curr = lexer;

    for (size_t i = 0; i < pp->lexer_curr.tda.count; i++) {
        if (pp->lexer_curr.tda.items[i].type == TT_INCLUDE) {
            if (i + 1 >= pp->lexer_curr.tda.count) {
                PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Expected file to include.");
            }

            i++;

            SV filepath_in = pp->lexer_curr.tda.items[i].value;

            if (sv_eq(filepath_in, filepath) == 0) {
                PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Cannot include a file in itself.");
            }

            Lexer prev_lexer = pp->lexer_curr;

            get_directives(pp, filepath_in, conf);

            Block include_block = {0};

            for (size_t j = 0; j < pp->lexer_curr.tda.count; j++) {
                da_push(include_block, pp->lexer_curr.tda.items[j]);
            }

            pp->lexer_curr = prev_lexer;

            hashmap_set(pp->includes, &(Include) {.key = filepath_in, .value = include_block});

        } else if (pp->lexer_curr.tda.items[i].type == TT_MACRO_DEF) {

            Token org = pp->lexer_curr.tda.items[i];

            if (i + 1 >= pp->lexer_curr.tda.count && pp->lexer_curr.tda.items[i + 1].type != TT_IDENTIFIER) {
                PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Expected Identifier for a macro.");
            }

            i++;

            SV ident = pp->lexer_curr.tda.items[i].value;

            Macro * check = (Macro*) hashmap_get(pp->macros, &(Macro){.key = ident});

            if(check){
                PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Cannot redefine macro: %.*s", ident.len, ident.data);
            }

            i++;

            Block macro_block = {0};
            ArgDA argDa = {0};

            for (; i < pp->lexer_curr.tda.count && pp->lexer_curr.tda.items[i].type != TT_MACRO_END; i++) {

                if (i == pp->lexer_curr.tda.count - 1 && pp->lexer_curr.tda.items[i].type != TT_MACRO_END) {
                    PANIC(GENERIC_ERROR, &org.loc, "Missing MACRO_END keyword.");
                }

                Token tok = pp->lexer_curr.tda.items[i];

                if (tok.type == TT_INCLUDE) {
                    PANIC(GENERIC_ERROR, &tok.loc, "Cannot put INCLUDE keyword inside macro definition.");
                }

                if(tok.type == TT_OPEN_SQUARE){

                    if (i + 1 >= pp->lexer_curr.tda.count && pp->lexer_curr.tda.items[i + 1].type != TT_IDENTIFIER) {
                        PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Expected Identifier for a macro argument.");
                    }

                    i++;

                    do{
                        tok = pp->lexer_curr.tda.items[i];

                        da_push(argDa, ((Arg){.key = tok.value}));

                        if(i+1 >= pp->lexer_curr.tda.count &&
                           pp->lexer_curr.tda.items[i + 1].type != TT_COMMA &&
                           pp->lexer_curr.tda.items[i + 1].type != TT_CLOSE_SQUARE){
                            PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Expected comma or close square bracket after macro argument");
                        }

                        i++;

                        tok = pp->lexer_curr.tda.items[i];

                        if(tok.type == TT_CLOSE_SQUARE){
                            break;
                        }

                        if(tok.type == TT_COMMA){
                            if (i + 1 >= pp->lexer_curr.tda.count && pp->lexer_curr.tda.items[i + 1].type != TT_IDENTIFIER) {
                                PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "Expected Identifier for a macro argument after comma.");
                            }

                            i++;
                        }

                        tok = pp->lexer_curr.tda.items[i];

                    }while(tok.type == TT_IDENTIFIER);
                }

                if(tok.type == TT_CLOSE_SQUARE) continue;

                da_push(macro_block, tok);
            }

            hashmap_set(pp->macros, &(Macro) {.key = ident, .value = macro_block, .argDa = argDa});
        } else if (pp->lexer_curr.tda.items[i].type == TT_IDENTIFIER) {

            if (i + 1 < pp->lexer_curr.tda.count && pp->lexer_curr.tda.items[i + 1].type == TT_COLON) {
                pp->is_head_label_set = true;
            }

        } else if (pp->lexer_curr.tda.items[i].type == TT_DOT) {

            if (!pp->is_head_label_set) {
                PANIC(GENERIC_ERROR, &pp->lexer_curr.tda.items[i].loc, "No top label defined to reference.");
            }

            if (i + 1 >= pp->lexer_curr.tda.count || pp->lexer_curr.tda.items[i + 1].type != TT_IDENTIFIER) {
                PANIC(SYNTAX_ERROR, &pp->lexer_curr.tda.items[i].loc, "Expected label after the dot");
            }

            i++;

        } else if (pp->lexer_curr.tda.items[i].type == TT_RET || pp->lexer_curr.tda.items[i].type == TT_END) {
            pp->is_head_label_set = false;
        }
    }

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Prepocess directive gathering for %.*s took %lf seconds.",
             filepath.len, filepath.data,
             (timespecDiff(&end, &start) * 0.000000001));
    }
}

void expand_dot(Prepass *pp, SV ident, Location loc){
    char * temp_buff = calloc(128, sizeof(char));

    for(size_t j = 0 ; j < pp->head_label.len; j++){
        temp_buff[j] = pp->head_label.data[j];
    }

    temp_buff[pp->head_label.len] = '_';

    for(size_t j = 0 ; j < ident.len; j++){
        temp_buff[pp->head_label.len + 1 + j] = ident.data[j];
    }

    Token new_ident = token(SV(temp_buff), TT_IDENTIFIER, loc);

    da_push(pp->tda, new_ident);
}

void expand_macro(Prepass * pp, Macro * mac){

    if(pp->curr_macro_depth >= 10){
        log_(ERROR, &pp->curr.loc, "Cannot expand this recursive macro.");
        PANIC(ERROR, &mac->value.items[0].loc, "Macro depth is greater than 10.");
    }

    pp->curr_macro_depth++;

    for(size_t i = 0; i < mac->value.count; i++){
        if(mac->value.items[i].type == TT_MACRO_DEF){
            pp->can_expand = false;
        }else if(mac->value.items[i].type == TT_MACRO_END && !pp->can_expand){
            pp->can_expand = true;
        }

        SV key = mac->value.items[i].value;
        Include * inc_in = (Include*) hashmap_get(pp->includes, &(Include){.key = key});
        Macro * mac_in = (Macro*) hashmap_get(pp->macros, &(Macro){.key = key});
        SpecialKeyWord  * skw = (SpecialKeyWord*) hashmap_get(pp->special_key_words,
                                                              &(SpecialKeyWord){.key = key});

        if(inc_in){
            PANIC(SYNTAX_ERROR, &pp->tda.items[pp->tda.count].loc, "Cannot have an include directive inside a macro.");
        }else if(mac_in && pp->can_expand){
            da_push(pp->expansionDA, mac->value.items[i].loc);
            if(mac_in->argDa.count > 0){
                pp->temp_block.count =0;
                if(i + mac_in->argDa.count >= mac->value.count){
                    PANIC(SYNTAX_ERROR, &mac->value.items[i].loc, "Expected %d arguments after macro %.*s", mac->argDa.count, key.len, key.data);
                }
                i++;
                for(size_t j = 0; j < mac_in->argDa.count; j++, i++){

                    bool is_arg = false; size_t arg_idx = 0;
                    for(size_t k= 0; k < mac->argDa.count; k++){
                        if(sv_eq(mac->value.items[i].value, mac_in->argDa.items[k].key) == 0){
                            is_arg = true;
                            arg_idx = k;
                        }
                    }

                    Token to_push = is_arg ? pp->temp_block.items[arg_idx] : mac->value.items[i];

                    da_push(pp->temp_block, to_push);
                }
                i--;
            }
            expand_macro(pp, mac_in);
        }else if(skw){
            switch(skw->id){
                case 0:{
                    char * line = malloc(10);
                    int n = sprintf(line, "%zu", pp->expansionDA.items[pp->expansionDA.count-1].line + 1);
                    line[n] = 0;
                    da_push(pp->tda, token(sv_from_cstr(line), TT_INTEGER_TYPE, mac->value.items[i].loc));
                }break;
                case 1:{
                    char * line = malloc(10);
                    int n = sprintf(line, "%zu", pp->expansionDA.items[pp->expansionDA.count-1].column);
                    line[n] = 0;
                    da_push(pp->tda, token(SV(line), TT_INTEGER_TYPE, mac->value.items[i].loc));
                }break;
                case 2:{
                    da_push(pp->tda, token(pp->expansionDA.items[pp->expansionDA.count-1].filepath, TT_STRING, mac->value.items[i].loc));
                }break;
            }
        }else{
            bool is_arg = false; size_t arg_idx = 0;
            for(size_t j = 0; j < mac->argDa.count; j++){
                if(sv_eq(key, mac->argDa.items[j].key) == 0){
                    is_arg = true;
                    arg_idx = j;
                }
            }

            Token to_push = is_arg ? pp->temp_block.items[arg_idx] : mac->value.items[i];

            Macro * inside_macro = (Macro*) hashmap_get(pp->macros, &(Macro){.key = to_push.value});

            if(inside_macro && pp->can_expand){
                Block tmp = pp->temp_block;
                da_push(pp->expansionDA, mac->value.items[i].loc);
                if(inside_macro->argDa.count > 0){
                    pp->temp_block.count =0;
                    if(i + inside_macro->argDa.count >= mac->value.count){
                        PANIC(SYNTAX_ERROR, &mac->value.items[i].loc, "Expected %d arguments after macro %.*s", mac->argDa.count, key.len, key.data);
                    }
                    i++;
                    for(size_t j = 0; j < inside_macro->argDa.count; j++, i++){

                        bool is_arg = false; size_t arg_idx = 0;
                        for(size_t k= 0; k < mac->argDa.count; k++){
                            if(sv_eq(mac->value.items[i].value, inside_macro->argDa.items[k].key) == 0){
                                is_arg = true;
                                arg_idx = k;
                            }
                        }

                        Token to_push = is_arg ? pp->temp_block.items[arg_idx] : mac->value.items[i];

                        da_push(pp->temp_block, to_push);
                    }
                    i--;
                }
                expand_macro(pp, inside_macro);
                pp->temp_block = tmp;
            }else{
                da_push(pp->tda, token_exp(to_push.value, to_push.type, to_push.loc, pp->expansionDA));
            }
        }
    }

    pp->curr_macro_depth--;
    pp->expansionDA.count--;
}

void expand_include(Prepass * pp, Include * inc){

    pp->curr_include_depth++;

    for (size_t i = 0; i < inc->value.count; i++) {

        if (inc->value.items[i].type == TT_MACRO_DEF) {
            pp->can_expand = false;
        } else if (inc->value.items[i].type == TT_MACRO_END && !pp->can_expand) {
            pp->can_expand = true;
        }

        SV key = inc->value.items[i].value;
        Include *inc_in = (Include *) hashmap_get(pp->includes, &(Include) {.key = key});
        Macro *mac_in = (Macro *) hashmap_get(pp->macros, &(Macro) {.key = key});
        Expanded_Include * exp = (Expanded_Include*) hashmap_get(pp->expanded_includes,
                                                                 &(Expanded_Include){.key = key});
        SpecialKeyWord  * skw = (SpecialKeyWord*) hashmap_get(pp->special_key_words,
                                                              &(SpecialKeyWord){.key = key});

        if (inc->value.items[i].type == TT_DOT) {

            if (i + 1 >= inc->value.count || inc->value.items[i + 1].type != TT_IDENTIFIER) {
                PANIC(SYNTAX_ERROR, &inc->value.items[i].loc, "Expected label after the dot");
            }

            i++;

            SV ident = inc->value.items[i].value;

            expand_dot(pp, ident, inc->value.items[i].loc);
        } else if (inc_in && (!exp || !exp->value)) {
            expand_include(pp, inc_in);
            hashmap_set(pp->expanded_includes, &(Expanded_Include){.key = key, .value = true});
        } else if (mac_in && pp->can_expand) {
            da_push(pp->expansionDA, inc->value.items[i].loc);
            if(mac_in->argDa.count > 0){
                pp->temp_block.count =0;
                if(i + mac_in->argDa.count >= inc->value.count){
                    PANIC(SYNTAX_ERROR, &inc->value.items[i].loc, "Expected %d arguments after macro %.*s", mac_in->argDa.count, key.len, key.data);
                }
                i++;
                for(size_t j = 0; j < mac_in->argDa.count; j++, i++){

                    bool is_arg = false; size_t arg_idx = 0;
                    for(size_t k= 0; k < mac_in->argDa.count; k++){
                        if(sv_eq(inc->value.items[i].value, mac_in->argDa.items[k].key) == 0){
                            is_arg = true;
                            arg_idx = k;
                        }
                    }

                    Token to_push = is_arg ? pp->temp_block.items[arg_idx] : inc->value.items[i];

                    da_push(pp->temp_block, to_push);
                }
                i--;
            }
            expand_macro(pp, mac_in);
        }else if(skw){
            switch(skw->id){
                case 0:{
                    char * line = malloc(10);
                    int n = sprintf(line, "%zu", inc->value.items[i].loc.line + 1);
                    line[n] = 0;
                    da_push(pp->tda, token(sv_from_cstr(line), TT_INTEGER_TYPE, inc->value.items[i].loc));
                }break;
                case 1:{
                    char * line = malloc(10);
                    int n = sprintf(line, "%zu", inc->value.items[i].loc.column);
                    line[n] = 0;
                    da_push(pp->tda, token(SV(line), TT_INTEGER_TYPE, inc->value.items[i].loc));
                }break;
                case 2:{
                    da_push(pp->tda, token(inc->value.items[i].loc.filepath, TT_STRING, inc->value.items[i].loc));
                }break;
            }
        }else {

            if (inc->value.items[i].type == TT_IDENTIFIER) {

                if (i + 1 < inc->value.count && inc->value.items[i + 1].type == TT_COLON) {
                    pp->head_label = inc->value.items[i].value;
                    pp->is_head_label_set = true;
                }

            }

            da_push(pp->tda, inc->value.items[i]);
        }
    }

    da_push(pp->tda, token(SV("INC_END"), TT_END_INCLUDE, pp->curr.loc));

    pp->curr_include_depth--;

}

void expand(Prepass * pp){

    pp->can_expand = true;

    for(size_t i = 0; i < pp->lexer_base.tda.count; i++){

        pp->curr = pp->lexer_base.tda.items[i];

        if(pp->lexer_base.tda.items[i].type == TT_MACRO_DEF){
            pp->can_expand = false;
        }else if(pp->lexer_base.tda.items[i].type == TT_MACRO_END && !pp->can_expand){
            pp->can_expand = true;
        }

        SV key = pp->lexer_base.tda.items[i].value;
        Include * inc = (Include*) hashmap_get(pp->includes, &(Include){.key = key});
        Macro * mac = (Macro*) hashmap_get(pp->macros, &(Macro){.key = key});
        Expanded_Include * exp = (Expanded_Include*) hashmap_get(pp->expanded_includes,
                                                                 &(Expanded_Include){.key = key});
        SpecialKeyWord  * skw = (SpecialKeyWord*) hashmap_get(pp->special_key_words,
                                                              &(SpecialKeyWord){.key = key});

        if(pp->lexer_base.tda.items[i].type == TT_DOT){

            if(i + 1 >= pp->lexer_base.tda.count || pp->lexer_base.tda.items[i+1].type != TT_IDENTIFIER){
                PANIC(SYNTAX_ERROR, &pp->lexer_base.tda.items[i].loc, "Expected label after the dot");
            }

            i++;

            SV ident = pp->lexer_base.tda.items[i].value;

            expand_dot(pp, ident, pp->lexer_base.tda.items[i].loc);

        }else if(inc){
            if(!exp || !exp->value){
                expand_include(pp, inc);
                hashmap_set(pp->expanded_includes, &(Expanded_Include){.key = key, .value = true});

            }else{
                i++;
            }
        }else if(mac && pp->can_expand){
            da_push(pp->expansionDA, pp->lexer_base.tda.items[i].loc);
            if(mac->argDa.count > 0){
                pp->temp_block.count =0;
                if(i + mac->argDa.count >= pp->lexer_base.tda.count){
                    PANIC(SYNTAX_ERROR, &pp->lexer_base.tda.items[i].loc, "Expected %d arguments after macro %.*s", mac->argDa.count, key.len, key.data);
                }
                i++;
                for(size_t j = 0; j < mac->argDa.count; j++, i++){

                    bool is_arg = false; size_t arg_idx = 0;
                    for(size_t k= 0; k < mac->argDa.count; k++){
                        if(sv_eq(key, mac->argDa.items[k].key) == 0){
                            is_arg = true;
                            arg_idx = k;
                        }
                    }

                    Token to_push = is_arg ? pp->temp_block.items[arg_idx] : pp->lexer_base.tda.items[i];

                    da_push(pp->temp_block, to_push);
                }
                i--;
            }
            expand_macro(pp, mac);
        }else if(skw){
            switch(skw->id){
                case 0:{
                    char * line = malloc(10);
                    int n = sprintf(line, "%zu", pp->lexer_base.tda.items[i].loc.line + 1);
                    line[n] = 0;
                    da_push(pp->tda, token(sv_from_cstr(line), TT_INTEGER_TYPE, pp->lexer_base.tda.items[i].loc));
                }break;
                case 1:{
                    char * line = malloc(10);
                    int n = sprintf(line, "%zu", pp->lexer_base.tda.items[i].loc.column);
                    line[n] = 0;
                    da_push(pp->tda, token(SV(line), TT_INTEGER_TYPE, pp->lexer_base.tda.items[i].loc));
                }break;
                case 2:{
                    da_push(pp->tda, token(pp->lexer_base.tda.items[i].loc.filepath, TT_STRING, pp->lexer_base.tda.items[i].loc));
                }break;
            }
        }else{

            if(pp->lexer_base.tda.items[i].type == TT_IDENTIFIER){

                if(i + 1 < pp->lexer_base.tda.count && pp->lexer_base.tda.items[i+1].type == TT_COLON){
                    pp->head_label = pp->lexer_base.tda.items[i].value;
                    pp->is_head_label_set = true;
                }

            }

            da_push(pp->tda, pp->lexer_base.tda.items[i]);
        }
    }

}

void preprocess(Prepass * pp, Configs conf){

    struct timespec start, end;


    get_directives(pp, conf.filepath, conf);

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    expand(pp);

    if(conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Expanding directives took %lf seconds.",
             (timespecDiff(&end, &start) * 0.000000001));
    }

    hashmap_free(pp->special_key_words);
    hashmap_free(pp->expanded_includes);
    hashmap_free(pp->includes);
    hashmap_free(pp->macros);

}


