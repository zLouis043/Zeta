#ifndef ZASM_FILEREADER_H
#define ZASM_FILEREADER_H

#include "common/SV.h"

void check_extension(SV filepath, char *extension);
int slurp_file(SV file_path, SV *content);

#endif //ZASM_FILEREADER_H
