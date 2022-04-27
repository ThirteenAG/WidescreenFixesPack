#include "log.h"
#include <string.h>
#ifndef NANOPRINTF_IMPLEMENTATION
#define NANOPRINTF_IMPLEMENTATION
#include "nanoprintf.h"
#endif

struct circular_buffer_t cb;

void cb_init(struct circular_buffer_t* cb, void* buf, size_t capacity, size_t sz)
{
    cb->buffer = buf;
    cb->buffer_end = (char*)cb->buffer + capacity * sz;
    cb->capacity = capacity;
    cb->count = 0;
    cb->sz = sz;
    cb->head = cb->buffer;
    cb->tail = cb->buffer;
}

void cb_push_back(struct circular_buffer_t* cb, const void* item)
{
    if (cb->buffer)
    {
        if (cb->count < cb->capacity)
        {
            memcpy(cb->head, item, cb->sz);
            cb->head = (char*)cb->head + cb->sz;
            if (cb->head == cb->buffer_end)
                cb->head = cb->buffer;
            cb->count++;
        }
        else
        {
            memcpy(cb->buffer, (char*)cb->buffer + cb->sz, (size_t)((char*)cb->buffer_end - (char*)cb->buffer - cb->sz));
            cb->head = (char*)cb->buffer_end - cb->sz;
            memcpy(cb->head, item, cb->sz);
            if (cb->head == cb->buffer_end)
                cb->head = cb->buffer;
            cb->count++;
        }
    }
}

void SetBuffer(void* buffer, size_t capacity, size_t elem_size)
{
    cb_init(&cb, buffer, capacity, elem_size);
}

void Write(char* message)
{
    if (cb.buffer)
        cb_push_back(&cb, message);
}

char logf_buffer[255] = { 1 };
void WriteFormatted(const char* format, ...)
{
    if (cb.buffer)
    {
        va_list val;
        int rv;
        va_start(val, format);
        rv = npf_vsnprintf(logf_buffer, sizeof(logf_buffer), format, val);
        logger.Write(logf_buffer);
        va_end(val);
    }
}

void ClearLog()
{
    if (cb.buffer)
        memset(cb.buffer, 0, cb.buffer_end - cb.buffer);
}

struct logger_t logger =
{
    .SetBuffer = SetBuffer,
    .Write = Write,
    .WriteF = WriteFormatted,
    .ClearLog = ClearLog,
};