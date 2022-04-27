#ifndef H_LOG
#define H_LOG
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include "nanoprintf.h"

struct circular_buffer_t
{
    void* buffer;     // data buffer
    void* buffer_end; // end of data buffer
    size_t capacity;  // maximum number of items in the buffer
    size_t count;     // number of items in the buffer
    size_t sz;        // size of each item in the buffer
    void* head;       // pointer to head
    void* tail;       // pointer to tail
};

void cb_init(struct circular_buffer_t* cb, void* buf, size_t capacity, size_t sz);
void cb_push_back(struct circular_buffer_t* cb, const void* item);

struct logger_t {
    void (*SetBuffer)(void* buffer, size_t capacity, size_t elem_size);
    void (*Write)(char* message);
    void (*WriteF)(const char* format, ...);
    void (*ClearLog)();
};
extern struct circular_buffer_t circular_buffer;
extern struct logger_t logger;
#endif
