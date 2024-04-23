#ifndef ZASM_CONFIGS_H
#define ZASM_CONFIGS_H

#include "SV.h"
#include <stdbool.h>

typedef struct Configs Configs;

struct Configs{
    SV filepath;
    SV output_path;
    bool dump_op_codes;
    bool time_exec;
    bool is_dgb;
};

#endif //ZASM_CONFIGS_H
