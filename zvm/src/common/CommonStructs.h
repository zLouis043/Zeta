#ifndef ZASM_COMMONSTRUCTS_H
#define ZASM_COMMONSTRUCTS_H

#include "SV.h"

typedef struct Location Location;

struct Location{
    SV filepath;
    SV lineSV;
    size_t line;
    size_t column;
};

typedef struct LocDA LocDA;

struct LocDA{
    Location * items;
    size_t count;
    size_t capacity;
};

#define STACK_CAP 640000
#define MEM_CAP 640000


#endif //ZASM_COMMONSTRUCTS_H
