#ifndef CONFIGFLAGS_H_
#define CONFIGFLAGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>  
#include <string.h>  
#include <stdint.h>
#include <limits.h>
#include <errno.h>

typedef union{
    bool as_bool;
    char * as_str;
    char as_char;
    uint64_t as_uint64;
    size_t as_size;
}F_Val;

typedef enum {
    F_BOOL,
    F_STRING,
    F_CHAR,
    F_UINT64,
    F_SIZE,
    F_COUNT
}F_Type;

typedef struct {

    const char * name;
    const char * description;

    F_Val in_val;
    F_Val def_val;

    F_Type type;

}Flag;

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif

typedef struct {

    Flag flags[FLAGS_CAP];
    size_t flags_count;

    int argc;
    char **argv;

}F_Context;

char * flag_shift_args(int * argc, char *** argv);
Flag * flag_new(F_Type type, const char* name, const char * description);
bool * flag_bool(const char* name, const char * description, bool default_value);
char ** flag_str(const char* name, const char * description, char * default_value);
char * flag_char(const char* name, const char * description, char  default_value);
uint64_t * flag_uint64(const char* name, const char * description, uint64_t  default_value);
size_t * flag_size(const char* name, const char * description, uint64_t  default_value);
bool flag_parse(int argc, char **argv);
void flag_print_flags(FILE * Stream);

#define flag_type(f_type, v_type) Flag * flag = flag_new(f_type, name, description);\
                                    flag->def_val.v_type = default_value;\
                                    flag->in_val.v_type = default_value;\
                                    return &flag->in_val.v_type;\

#endif // CONFIGFLAG_H_
