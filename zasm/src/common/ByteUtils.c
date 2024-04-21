#include "ByteUtils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char * u8_byte_to_str(u8 byte){
    char * str = malloc(3);

    sprintf(str, "%02x", byte);

    str[2]  = 0;

    return str;
}

char * u_bytes_to_str(u8 * bytes, size_t size){

    char * str = malloc((size * 2)+1);

    char tmp[2];

    for(size_t i = 0; i < size * 2; i++){

        sprintf(tmp, "%02x", bytes[i]);
        memcpy(str + (i * 2), tmp, 2);

    }

    str[size * 2] = 0;

    return str;

}

char * s_bytes_to_str(s8 * bytes, size_t size){

    char * str = malloc((size * 2)+1);

    char tmp[2];

    for(size_t i = 0; i < size * 2; i++){

        sprintf(tmp, "%02x", bytes[i]);
        memcpy(str + (i * 2), tmp, 2);

    }

    str[size * 2] = 0;

    return str;

}

ByteDA no_val(){

    u8 b[1];
    b[0] = 0;

    return (ByteDA){
        .items = b,
        .count = 0,
        .capacity = 16
    };
}