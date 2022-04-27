#ifndef RINI_H
#define RINI_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PTR_NOT_END(a) (*(a) != '\r' && *(a) != '\n' && *(a) != 0)

#define CLEAN_PTR(a, b, c)  do { \
                                memset(a, 0, b); \
                                (c) = (char*)(a); \
                            } while (0)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define MAX_NAME 256
#define MAX_INT_STR_SIZE 32
#define MAX_BOOL_KEY_SIZE 6
#define MAX_LINE_SIZE(x) ((MAX_NAME + x) + 1)

typedef enum _value_types_t {
    STRING_VAL,
    INT_VAL,
    BOOL_VAL
} value_types_t;

typedef enum _parser_flags_t  {
    FLAGS_DEFAULT,
    EXPECT_QUOTE,
    LAST_ESCAPE_CHAR,
    ESCAPE_NOT_FOUND,
    END_EARLY,
    INT_NEG_NUMB
} parser_flags_t;

typedef enum _bool_type_t {
    BOOL_KEY_FALSE = 0,
    BOOL_KEY_TRUE = 1,
    BOOL_KEY_ERROR = 2
} bool_type_t;

typedef struct _bool_key_record_t {
    bool_type_t val;
    char* key;
    unsigned size;
} bool_keys_t;

int rini_get_key(const char* parent, const char* key, const char* config, unsigned config_size, const void* out, unsigned out_size, value_types_t type);

#endif // RINI_H
