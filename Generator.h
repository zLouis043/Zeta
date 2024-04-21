#ifndef ZASM_GENERATOR_H
#define ZASM_GENERATOR_H

#include "common/Configs.h"
#include "common/Hashmap.h"

typedef struct Generator Generator;

struct Generator{
    Configs conf;
    struct hashmap * file_paths;
};

void generator_init(Generator * gen, Configs conf);
int generate(Generator * gen);

#endif //ZASM_GENERATOR_H
