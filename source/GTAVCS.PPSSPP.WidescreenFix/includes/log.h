#pragma once
#include <pspkerneltypes.h>

struct logger_t {
    SceUID log_uid;
    void (*Write)(char* path, char* message);
    void (*Close)();
};
extern struct logger_t logger;