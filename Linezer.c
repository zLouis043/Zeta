#include "Linezer.h"

#include "common/DyArray.h"
#include "FileReader.h"

#include <stdio.h>
#include <stdlib.h>

#include "common/Log.h"

int get_lines_from_file(SV filepath, LinesDA * ldar){

    SV content  = {0};

    if(slurp_file(filepath, &content) < 0){
        PANIC(GENERIC_ERROR, NULL, "Could not read file %s", CSTR(filepath));
    }

    LinesDA lda = {0};

    while(content.len > 0){

        SV line = sv_chop_delim_l(&content, '\n');
        da_push(lda, line);

    }

    *ldar = lda;
    return 0;

}