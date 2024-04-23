#ifndef ZASM_SV_H
#define ZASM_SV_H

#include <stdlib.h>

typedef struct SV SV;

struct SV{
    char * data;
    size_t len;
};

#define CSTR(sv) sv_to_cstr((sv))
#define SV(cstr) sv_from_cstr((cstr))

SV sv_from_parts(char * str, size_t len);
SV sv_from_cstr(char * str);
char * sv_to_cstr(SV sv);
SV sv_chop_delim_l(SV * sv, char del);
int sv_eq(SV s1, SV s2);
SV sv_substring(SV sv, size_t start, size_t end);

#endif //ZASM_SV_H
