#include "Generator.h"
#include "common/Log.h"
#include "FileReader.h"
#include "Parser.h"

#include <stdio.h>
#include <string.h>

typedef struct FilePath FilePath;

struct FilePath{
    SV key;
    size_t value;
};

int fp_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const FilePath *ua = a;
    const FilePath *ub = b;
    return sv_eq(ua->key, ub->key);
}

uint64_t fp_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const FilePath *kw = item;
    return hashmap_sip(CSTR(kw->key), kw->key.len, seed0, seed1);
}

void generator_init(Generator * gen, Configs conf){
    gen->conf = conf;
    gen->file_paths = hashmap_new(sizeof(FilePath), 0, 0, 0,
                                  fp_hash, fp_compare, NULL, NULL);
}

SV replace_extension(SV base, SV new_extension){

    size_t i = 0;
    size_t len = base.len;
    char tmp[255];

    memset(tmp, 0, 255);
    memcpy(tmp, CSTR(base), len);

    while(tmp[len - i] != '.' && i < len) i++;
    size_t dot_pos = len - i + 1;

    memcpy(tmp + dot_pos, CSTR(new_extension), 6);
    return sv_from_cstr(tmp);
}

static int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p){
    return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

#include <time.h>

int generate(Generator * gen){
    Parser parser = {0};

    parser_init(&parser, gen->conf);

    int exit_code = parse(&parser);
    if(exit_code != 0){
        return exit_code;
    }
    struct timespec start, end;

    if(gen->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &start);
    }

    for(size_t i = 0; i < parser.fda.count; i++){
        hashmap_set(gen->file_paths, &(FilePath){.key = parser.fda.items[i], .value = i});
    }

    if(gen->conf.dump_op_codes) {
        printf("DATA: {\n");
        printf("\tEntry Point: NONE\n");
        printf("\tChunk Size: %zu\n", parser.data_chunk_da.count);
        printf("\tChunk Items: {\n");
        for (size_t i = 0; i < parser.data_chunk_da.count; i++) {
            printf("\t\t[%3zu] ", i);
            op_print(parser.data_chunk_da.items[i]);
        }
        printf("\t}\n}\n");

        printf("CODE: {\n");
        printf("\tEntry Point: %zu\n", parser.entry);
        printf("\tChunk Size: %zu\n", parser.code_chunk_da.count);
        printf("\tChunk Items:\n");
        for (size_t i = 0; i < parser.code_chunk_da.count; i++) {
            printf("\t\t[%3zu] ", i);
            op_print(parser.code_chunk_da.items[i]);
        }
        printf("\t}\n}\n");
    }

    if(gen->conf.output_path.data == NULL){
        gen->conf.output_path = replace_extension(gen->conf.filepath, SV("zexec"));
        check_extension(gen->conf.output_path, "zexec");
    }else{
        check_extension(gen->conf.output_path, "zexec");
    }

    char output[255];
    memcpy(output, gen->conf.output_path.data, gen->conf.output_path.len);

    FILE* fp = fopen(output, "wb");

    if(!fp){
        PANIC(GENERIC_ERROR, NULL, "Could not open file %.*s\n", gen->conf.output_path.len, gen->conf.output_path.data);
    }

    size_t sizeof_cap = sizeof(size_t);
    size_t sizeof_op = sizeof(u8);
    size_t sizeof_one = sizeof(u8);
    size_t sizeof_two = sizeof(short);
    size_t sizeof_four = sizeof(int);
    size_t sizeof_eight = sizeof(double);
    size_t sizeof_tag = sizeof(char) * 10;
    size_t bytes_written = 0;

    size_t mem_cap = MEM_CAP;
    size_t stack_cap = STACK_CAP;

    char TAG[] = "ZEXEC";

    bytes_written += fwrite((const void*)&TAG, sizeof_one, 5, fp);

    bytes_written += fwrite((const void*)&gen->conf.is_dgb, sizeof_one, 1, fp);

    if(gen->conf.is_dgb){
        bytes_written += fwrite((const void*)&parser.fda.count, 1, sizeof_cap, fp);
        for(size_t i = 0; i < parser.fda.count; i++){
            bytes_written += fwrite((const void*)&parser.fda.items[i].len, 1, sizeof_cap, fp);
            for(size_t j = 0; j < parser.fda.items[i].len; j++){
                char c = parser.fda.items[i].data[j];
                bytes_written += fwrite((const void*)&c, 1, 1, fp);
            }
        }
    }

    bytes_written += fwrite((const void*)&mem_cap, 1, sizeof_cap, fp);
    bytes_written += fwrite((const void*)&stack_cap, 1, sizeof_cap, fp);

    char DATA_CHUNK_NAME[11] = "DATA_CHUNK";
    bytes_written += fwrite((const void*)&DATA_CHUNK_NAME, 1, sizeof_tag, fp);

    size_t chunk_count = parser.data_chunk_da.count;
    bytes_written += fwrite((const void*)&chunk_count, 1, sizeof_cap, fp);
    if(parser.data_chunk_da.count != 0) {
        for (size_t i = 0; i < parser.data_chunk_da.count; i++) {
            OPCODE op = parser.data_chunk_da.items[i];
            bytes_written += fwrite((const void *) &op.op, sizeof_op, 1, fp);
            if(gen->conf.is_dgb){

                bytes_written += fwrite((const void*)&op.is_expanded, 1, sizeof_op, fp);
                if(op.is_expanded){
                    size_t exp_count = op.expDA.count;
                    bytes_written += fwrite((const void*)&exp_count, 1, sizeof_cap, fp);
                    for(size_t j = 0; j < exp_count; j++) {
                        FilePath *filePath = (FilePath *) hashmap_get(gen->file_paths,
                                                                      &(FilePath) {.key = op.expDA.items[j].filepath});
                        if (!filePath) {
                            PANIC(GENERIC_ERROR, NULL, "Generation failed. File not found: %*.s",
                                  op.expDA.items[j].filepath.len, op.expDA.items[j].filepath.data);
                        }
                        size_t idx = filePath->value;
                        bytes_written += fwrite((const void *) &idx, 1, 1, fp);
                        short line = (short)op.expDA.items[j].line;
                        short col = (short)op.expDA.items[j].column;
                        bytes_written += fwrite((const void *) &line, 1, sizeof_two, fp);
                        bytes_written += fwrite((const void *) &col, 1, sizeof_two, fp);
                    }
                }

                FilePath * filePath = (FilePath*) hashmap_get(gen->file_paths, &(FilePath){.key = op.loc.filepath});
                if(!filePath){
                    PANIC(GENERIC_ERROR, NULL, "Generation failed. File not found: %*.s", op.loc.filepath.len, op.loc.filepath.data);
                }
                size_t idx = filePath->value;
                bytes_written += fwrite((const void *)&idx, 1, 1, fp);
                short line = (short)op.loc.line;
                short col = (short)op.loc.column;
                bytes_written += fwrite((const void *)&line, 1, sizeof_two, fp);
                bytes_written += fwrite((const void *)&col, 1, sizeof_two, fp);
            }
            bytes_written += fwrite((const void*)&op.t, 1, sizeof_op, fp);
            switch (op.t) {
                case CHAR_: {
                    char d = (char)op.as.one;
                    bytes_written += fwrite((const void *) &d, 1, sizeof_one, fp);
                }
                    break;
                case PTR_: {
                    short d = op.as.two;
                    bytes_written += fwrite((const void *) &d, 1, sizeof_two, fp);
                }
                    break;
                case INTEGER_: {
                    int d = op.as.four;
                    bytes_written += fwrite((const void *) &d, 1, sizeof_four, fp);
                }
                    break;
                case DOUBLE_: {
                    double d = op.as.eight;
                    bytes_written += fwrite((const void *) &d, 1, sizeof_eight, fp);
                }
                    break;
                case BYTE_ARRAY_: {
                    size_t count_bda = op.as.bda.count;
                    bytes_written += fwrite((const void *) &count_bda, 1, sizeof_cap, fp);
                    for (size_t j = 0; j < op.as.bda.count; j++) {
                        u8 c = op.as.bda.items[j];
                        bytes_written += fwrite((const void *) &c, 1, sizeof_one, fp);
                    }
                }
                    break;
                case BYTE_: {
                    char d = (char)op.as.one;
                    bytes_written += fwrite((const void *) &d, 1, sizeof_one, fp);
                }break;
                case SHORT_: {
                    short d = op.as.two;
                    bytes_written += fwrite((const void *) &d, 1, sizeof_two, fp);
                }break;
                case NONE_:
                    break;
                default: break;
            }
        }

    }
    char CODE_CHUNK_NAME[11] = "CODE_CHUNK";
    bytes_written += fwrite((const void*)&CODE_CHUNK_NAME, 1, sizeof_tag, fp);

    bytes_written += fwrite((const void*)&parser.code_chunk_da.count, 1, sizeof_cap, fp);
    bytes_written += fwrite((const void*)&parser.entry, 1, sizeof_cap, fp);
    for(size_t i = 0; i < parser.code_chunk_da.count; i++){
        OPCODE op = parser.code_chunk_da.items[i];
        bytes_written += fwrite((const void*)&op.op, 1,sizeof_op,fp);
        if(gen->conf.is_dgb){

            bytes_written += fwrite((const void*)&op.is_expanded, 1, sizeof_op, fp);
            if(op.is_expanded){
                size_t exp_count = op.expDA.count;
                bytes_written += fwrite((const void*)&exp_count, 1, sizeof_cap, fp);
                for(size_t j = 0; j < exp_count; j++) {
                    FilePath *filePath = (FilePath *) hashmap_get(gen->file_paths,
                                                                  &(FilePath) {.key = op.expDA.items[j].filepath});
                    if (!filePath) {
                        PANIC(GENERIC_ERROR, NULL, "Generation failed. File not found: %*.s",
                              op.expDA.items[j].filepath.len, op.expDA.items[j].filepath.data);
                    }
                    size_t idx = filePath->value;
                    bytes_written += fwrite((const void *) &idx, 1, 1, fp);
                    short line = (short)op.expDA.items[j].line;
                    short col = (short)op.expDA.items[j].column;
                    bytes_written += fwrite((const void *) &line, 1, sizeof_two, fp);
                    bytes_written += fwrite((const void *) &col, 1, sizeof_two, fp);
                }
            }

            FilePath * filePath = (FilePath*) hashmap_get(gen->file_paths, &(FilePath){.key = op.loc.filepath});
            if(!filePath){
                PANIC(GENERIC_ERROR, NULL, "Generation failed. File not found: %*.s", op.loc.filepath.len, op.loc.filepath.data);
            }
            size_t idx = filePath->value;
            bytes_written += fwrite((const void *)&idx, 1, sizeof_one, fp);
            short line = (short)op.loc.line;
            short col = (short)op.loc.column;
            bytes_written += fwrite((const void *)&line, 1, sizeof_two, fp);
            bytes_written += fwrite((const void *)&col, 1, sizeof_two, fp);
        }
        bytes_written += fwrite((const void*)&op.t, 1, sizeof_op, fp);
        switch(op.t){
            case CHAR_: {
                char d = (char)op.as.one;
                bytes_written += fwrite((const void *) &d, 1, sizeof_one, fp);
            }
                break;
            case PTR_: {
                short d = op.as.two;
                bytes_written += fwrite((const void *) &d, 1, sizeof_two, fp);
            }
                break;
            case INTEGER_: {
                int d = op.as.four;
                bytes_written += fwrite((const void *) &d, 1, sizeof_four, fp);
            }
                break;
            case DOUBLE_: {
                double d = op.as.eight;
                bytes_written += fwrite((const void *) &d, 1, sizeof_eight, fp);
            }
                break;
            case BYTE_ARRAY_: {
                size_t count_bda = op.as.bda.count;
                bytes_written += fwrite((const void *) &count_bda, 1, sizeof_cap, fp);
                for (size_t j = 0; j < op.as.bda.count; j++) {
                    u8 c = op.as.bda.items[j];
                    bytes_written += fwrite((const void *) &c, 1, sizeof_one, fp);
                }
            }
                break;
            case BYTE_: {
                char d = (char)op.as.one;
                bytes_written += fwrite((const void *) &d, 1, sizeof_one, fp);
            }break;
            case SHORT_: {
                short d = op.as.two;
                bytes_written += fwrite((const void *) &d, 1, sizeof_two, fp);
            }break;
            case NONE_:
                break;
            default: break;
        }
    }
    fclose(fp);

    if(gen->conf.time_exec) {
        clock_gettime(CLOCK_MONOTONIC, &end);
        log_(INFO, NULL, "Generation took %lf seconds.",
             (timespecDiff(&end, &start) * 0.000000001));
    }

    hashmap_free(gen->file_paths);
    return exit_code;
}
