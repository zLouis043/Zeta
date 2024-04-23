#include "ConfigFlags.h"

static F_Context g_flag_context;

char * flag_shift_args(int * argc, char *** argv){

    assert(*argc > 0);
    char * result = **argv;
    *argc -= 1;
    *argv += 1;
    return result;

}

Flag * flag_new(F_Type type, const char* name, const char * description){

    F_Context *c = &g_flag_context;

    assert(c->flags_count < FLAGS_CAP);
    Flag * flag = &c->flags[c->flags_count++];
    memset(flag, 0, sizeof(Flag));
    flag->type = type;
    flag->name = name;
    flag->description = description;
    return flag;

}

bool * flag_bool(const char* name, const char * description, bool default_value){

    flag_type(F_BOOL, as_bool);

}

char ** flag_str(const char* name, const char * description, char * default_value){

    flag_type(F_STRING, as_str);

}

char * flag_char(const char* name, const char * description, char  default_value){

    flag_type(F_CHAR, as_char);

}

uint64_t * flag_uint64(const char* name, const char * description, uint64_t default_value){

    flag_type(F_UINT64, as_uint64);

}

size_t * flag_size(const char* name, const char * description, uint64_t  default_value){

    flag_type(F_SIZE, as_size);

}

bool flag_parse(int argc, char **argv){

    F_Context *c = &g_flag_context;

    //flag_shift_args(&argc, &argv);

    while(argc > 0){
        char * flag = flag_shift_args(&argc, &argv);

        if(*flag != '-'){
            c->argc = argc + 1;
            c->argv = argv - 1;
            return true;
        }

        if (strcmp(flag, "--") == 0) {
            c->argc = argc;
            c->argv = argv;
            return true;
        }

        flag += 1;

        bool found = false;

        for(size_t i = 0; i < c->flags_count; i++){

            if(strncmp(c->flags[i].name, flag, strlen(c->flags[i].name)) == 0){
                switch (c->flags[i].type) {
                    case F_BOOL: {

                        /*if(strcmp(argv[0], "true") == 0 || strcmp(argv[0], "false") == 0){

                            char * arg = flag_shift_args(&argc, &argv);

                            c->flags[i].in_val.as_bool = (strcmp(arg, "true") == 0) ? true : false;

                        }else {

                            c->flags[i].in_val.as_bool = true;

                        }*/
                        c->flags[i].in_val.as_bool = true;

                    }break;
                    case F_STRING: {

                        if(argc == 0){
                            fprintf(stderr, "[ERROR] no value specified for flag '%s'\n", flag);
                            return false;
                        }

                        char* arg = flag_shift_args(&argc, &argv);

                        c->flags[i].in_val.as_str = arg;
                    }break;
                    case F_CHAR: {
                        if(argc == 0){
                            fprintf(stderr, "[ERROR] no value specified for flag '%s'\n", flag);
                            return false;
                        }

                        char * arg = flag_shift_args(&argc, &argv);

                        c->flags[i].in_val.as_char = *arg;
                    }break;
                    case F_UINT64: {

                        if(argc == 0){
                            fprintf(stderr, "[ERROR] no value specified for flag '%s'\n", flag);
                            return false;
                        }

                        char* arg = flag_shift_args(&argc, &argv);

                        char *endptr;
                        unsigned long long int result = strtoull(arg, &endptr, 10);

                        if (result == ULLONG_MAX && errno == ERANGE) {
                            fprintf(stderr, "[ERROR] integer overflow : '%s'\n", flag);
                            return false;
                        }

                        c->flags[i].in_val.as_uint64 = result;
                    }break;
                    case F_SIZE: {

                        if(argc == 0){
                            fprintf(stderr, "[ERROR] no value specified for flag '%s'\n", flag);
                            return false;
                        }

                        char* arg = flag_shift_args(&argc, &argv);

                        char *endptr;
                        unsigned long long int result = strtoull(arg, &endptr, 10);

                        if (strcmp(endptr, "K") == 0) {
                            result *= 1024;
                        } else if (strcmp(endptr, "M") == 0) {
                            result *= 1024*1024;
                        } else if (strcmp(endptr, "G") == 0) {
                            result *= 1024*1024*1024;
                        } else if (strcmp(endptr, "") != 0) {
                            fprintf(stderr, "[ERROR] wrong suffix used for size '%s'\n", endptr);
                            return false;
                        }

                        if(*(endptr + 1) != '\0'){
                            fprintf(stderr, "[ERROR] invalid number '%s'\n", flag);
                            return false;
                        }

                        if (result == ULLONG_MAX && errno == ERANGE) {
                            fprintf(stderr, "[ERROR] integer overflow : '%s'\n", flag);
                            return false;
                        }

                        c->flags[i].in_val.as_size = result;
                    }break;
                    default:{
                        assert(0 && "unreachable");
                        exit(EXIT_FAILURE);
                    }
                }
                found = true;
            }
        }

        if (!found) {
            fprintf(stderr, "[ERROR] unknown flag:  '%s'\n", flag);
            return false;
        }

    }

    c->argc = argc;
    c->argv = argv;
    return true;
}

void flag_print_flags(FILE* Stream){

    F_Context *c = &g_flag_context;

    fprintf(Stream, "[INFO] Flags: \n");

    for (size_t i = 0; i < c->flags_count; ++i) {

        Flag *flag = &c->flags[i];

        switch (c->flags[i].type) {
            case F_BOOL:{
                fprintf(Stream, "[INFO]    Name: -%s \n", flag->name);
                fprintf(Stream, "[INFO]        Description: %s\n", flag->description);
                fprintf(Stream, "[INFO]        Default: %s\n", flag->def_val.as_bool ? "true" : "false");
            }break;
            case F_STRING:{
                fprintf(Stream, "[INFO]    Name: -%s <string>\n", flag->name);
                fprintf(Stream, "[INFO]        Description: %s\n", flag->description);
                if (flag->def_val.as_str) {
                    fprintf(Stream, "[INFO]        Default: \"%s\"\n", flag->def_val.as_str);
                }else {
                    fprintf(Stream, "[INFO]        Default: (null)\n");
                }
            }break;
            case F_CHAR:{
                fprintf(Stream, "[INFO]    Name: -%s <char>\n", flag->name);
                fprintf(Stream, "[INFO]        Description: %s\n", flag->description);
                fprintf(Stream, "[INFO]        Default: '%c'\n", flag->def_val.as_char);
            }break;
            case F_UINT64:{
                fprintf(Stream, "[INFO]    Name: -%s <uint64_t number>\n", flag->name);
                fprintf(Stream, "[INFO]        Description: %s\n", flag->description);
                fprintf(Stream, "[INFO]        Default: %llu\n", flag->def_val.as_uint64);
            }break;
            case F_SIZE:{
                fprintf(Stream, "[INFO]    Name: -%s <uint64_t number[K | M | G]>\n", flag->name);
                fprintf(Stream, "[INFO]        Description: %s\n", flag->description);
                fprintf(Stream, "[INFO]        Default: %zu\n", flag->def_val.as_size);
            }break;
            default:{
                assert(0 && "unreachable");
                exit(EXIT_FAILURE);
            }break;
        }

    }
}



