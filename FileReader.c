#include "FileReader.h"

#include <stdio.h>

#include "common/Log.h"

#define ZASM_EXTENSION "zasm"

void check_extension(SV filepath, char *extension){

    size_t i = 0;
    while(filepath.data[filepath.len - i] != '.' && i < filepath.len) i++;

    SV extSV = sv_from_parts(filepath.data + filepath.len - i + 1, i -1);

    if(sv_eq(extSV, SV(extension)) != 0){
        PANIC(GENERIC_ERROR, NULL, "Wrong extension for file %s. Extension needed %s.", CSTR(filepath), extension);
    }

}

int slurp_file(SV file_path, SV *content){

    check_extension(file_path, ZASM_EXTENSION);

    const char *file_path_cstr = CSTR(file_path);

    FILE *f = fopen(file_path_cstr, "rb");
    if (f == NULL) {
        return -1;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        return -1;
    }

    long m = ftell(f);
    if (m < 0) {
        return -1;
    }

    char *buffer = realloc(NULL, (size_t) m);
    if (buffer == NULL) {
        return -1;
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        return -1;
    }

    size_t n = fread(buffer, 1, (size_t) m, f);
    if (ferror(f)) {
        return -1;
    }

    fclose(f);

    if(n == 0){
        PANIC(GENERIC_ERROR, NULL, "The file %s is empty.", CSTR(file_path));
    }

    if (content) {
        content->len = n;
        content->data = buffer;
    }

    return 0;
}