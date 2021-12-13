#pragma once
#include <stdarg.h>
#include <pspkerneltypes.h>

struct logger_t {
    SceUID log_uid;
    char* path;
    void (*SetPath)(const char* szFileName);
    void (*Write)(char* message);
    void (*WriteF)(const char* format, ...);
    void (*Close)();
};
extern struct logger_t logger;