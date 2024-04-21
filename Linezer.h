#ifndef ZASM_LINEZER_H
#define ZASM_LINEZER_H
#define LINES_CAP  16

#include "common/SV.h"

typedef struct ld{
    SV * items;
    size_t count;
    size_t capacity;
}LinesDA;

int get_lines_from_file(SV filepath, LinesDA * ldar);

#endif //ZASM_LINEZER_H
