#include "SV.h"

#include <stdio.h>
#include <string.h>

SV sv_from_parts(char * str, size_t len){
    return (SV){
        .data = str,
        .len = len
    };
}

SV sv_from_cstr(char * str){
    return sv_from_parts(str, strlen(str));
}

SV sv_chop(SV * sv, size_t n){

    if (n > sv->len) {
        n = sv->len;
    }

    SV result = sv_from_parts(sv->data, n);

    sv->data  += n;
    sv->len -= n;

    return result;

}

SV sv_chop_delim_l(SV * sv, char delim){

    size_t i = 0;
    while (i < sv->len && sv->data[i] != delim) {
        i += 1;
    }

    SV result = sv_from_parts(sv->data, i);

    if (i < sv->len) {
        sv->len -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->len -= i;
        sv->data  += i;
    }

    return result;

}

char * sv_to_cstr(SV sv){

    char * str = malloc(sv.len + 1);

    memcpy(str, sv.data, sv.len);
    str[sv.len] = 0;
    return str;

}

int sv_eq(SV s1, SV s2){

    if(s1.len != s2.len) return -1;

    return memcmp(s1.data, s2.data, s1.len);

}

SV sv_substring(SV sv, size_t start, size_t end){
    return sv_from_parts(sv.data + start, end - start);
}