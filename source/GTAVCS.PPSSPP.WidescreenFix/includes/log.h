#pragma once

struct logger_t {
    int init;
    void (*Write)(char* path, char* message);
};
extern struct logger_t logger;