#ifndef ZASM_BYTEUTILS_H
#define ZASM_BYTEUTILS_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef double s64;

// LITTLE - ENDIAN

#define to_u8(value) ((u8 *) &value);
#define from_u8(type, value) (*((type *)value))

#define to_s8(value) ((s8 *) &value);
#define from_s8(type, value) (*((type *)value))

#define print_bytes(type, value) \
    do{                          \
        for(size_t i = 0; i < sizeof(type); i++) \
                printf("%02x", value[i]);        \
        printf("\n");                            \
    }while(0)

char * u8_byte_to_str(u8 byte);
char * u_bytes_to_str(u8 * bytes, size_t size);
char * s_bytes_to_str(s8 * bytes, size_t size);

#define u8_to_str(bytes)    u_byte_to_str(bytes);
#define u16_to_str(bytes)   u_bytes_to_str(bytes, sizeof(u16));
#define u32_to_str(bytes)   u_bytes_to_str(bytes, sizeof(u32));
#define u64_to_str(bytes)   u_bytes_to_str(bytes, sizeof(u64));

#define s8_to_str(bytes)    s_bytes_to_str(bytes, sizeof(s8));
#define s16_to_str(bytes)   s_bytes_to_str(bytes, sizeof(s16));
#define s32_to_str(bytes)   s_bytes_to_str(bytes, sizeof(s32));
#define s64_to_str(bytes)   s_bytes_to_str(bytes, sizeof(s64));

typedef struct ByteDA ByteDA;

struct ByteDA{
    u8 * items;
    size_t count;
    size_t capacity;
};

ByteDA no_val();

#endif //ZASM_BYTEUTILS_H
