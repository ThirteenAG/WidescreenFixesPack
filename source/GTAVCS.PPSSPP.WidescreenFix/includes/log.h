#pragma once
#include <stdarg.h>
#include <pspkerneltypes.h>

struct logger_t {
    SceUID log_uid;
    void (*Write)(char* path, char* message);
    void (*WriteF)(char* path, const char* format, ...);
    void (*Close)();
};
extern struct logger_t logger;