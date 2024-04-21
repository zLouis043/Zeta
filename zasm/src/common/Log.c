#include "Log.h"

#include <stdio.h>
#include <stdarg.h>

const char * log_cstr[LOG_COUNT] = {
        [DEBUG]  = "DEBUG",
        [TRACE]  = "TRACE",
        [INFO] = "INFO",
        [GENERIC_ERROR]  = "ERROR",
        [LEXING_ERROR]  = "LEXING ERROR",
        [SYNTAX_ERROR]  = "SYNTAX ERROR",
        [STACK_TRACE]   = "STACK TRACE",
        [RUNTIME_ERROR] = "RUNTIME ERROR"
};

void log_(LogLevel log_level, Location * loc, char * fmt, ...){

    va_list args;

    va_start(args,fmt);

    char buffer[1024];

    vsprintf(buffer, fmt, args);

    if(log_level > 2){
        fprintf(stderr, "ZASM: [%s]", log_cstr[log_level]);

        if(loc){
            fprintf(stderr, " %s:%zu:%zu >", CSTR(loc->filepath), loc->line + 1, loc->column);
        }

        fprintf(stderr, " %s\n", buffer);

        if(loc && loc->lineSV.data != NULL){
            char line_buff[16];
            int line_padd = sprintf(line_buff, "%5zu", loc->line + 1) + 2;

            fprintf(stderr, "%5zu | %s\n", loc->line + 1, CSTR(loc->lineSV));

            fprintf(stderr, "%*.s |", line_padd - 2, " ");
            for(size_t i = 0; i < loc->column; i++){
                fprintf(stderr, " ");
            }

            fprintf(stderr, "^\n");

        }

    }else{
        fprintf(stdout, "ZASM: [%s]", log_cstr[log_level]);

        if(loc){
            fprintf(stdout, " %s:%zu:%zu >", CSTR(loc->filepath), loc->line + 1, loc->column);
        }

        fprintf(stdout, " %s\n", buffer);

        if(loc && loc->lineSV.data != NULL){
            char line_buff[16];
            int line_padd = sprintf(line_buff, "%5zu", loc->line + 1) + 2;

            fprintf(stdout, "%5zu | %s\n", loc->line + 1, CSTR(loc->lineSV));

            fprintf(stdout, "%*.s |", line_padd - 2, " ");
            for(size_t i = 0; i < loc->column; i++){
                fprintf(stdout, " ");
            }

            fprintf(stdout, "^\n");

        }

    }


}