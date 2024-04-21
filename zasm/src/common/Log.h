#ifndef ZASM_LOG_H
#define ZASM_LOG_H

typedef enum LogLevel LogLevel;

#include "CommonStructs.h"

enum LogLevel{
    DEBUG,
    TRACE,
    INFO,
    GENERIC_ERROR,
    LEXING_ERROR,
    SYNTAX_ERROR,
    RUNTIME_ERROR,
    STACK_TRACE,
    LOG_COUNT
};

void log_(LogLevel log_level, Location * loc, char * fmt, ...);

#define PANIC(LOG_LEVEL, LOC, ...) \
    do{                                \
        log_(LOG_LEVEL, LOC, __VA_ARGS__);                                   \
        exit(-1);                               \
    }while(0)
#endif //ZASM_LOG_H
