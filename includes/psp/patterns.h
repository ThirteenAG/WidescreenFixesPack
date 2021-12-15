#pragma once
#include <stddef.h>
#include <stdint.h>

struct range_pattern_t {
    uintptr_t(*get)(size_t count, uintptr_t range_start, size_t range_size, const char* pattern_str, int32_t offset);
    uintptr_t(*get_first)(uintptr_t range_start, size_t range_size, const char* pattern_str, int32_t offset);
};
extern struct range_pattern_t range_pattern;

struct pattern_t {
    uintptr_t text_addr;
    size_t text_size;
    void (*SetGameBaseAddress)(uintptr_t addr, size_t size);
    uintptr_t(*get)(size_t count, const char* pattern_str, int32_t offset);
    uintptr_t(*get_first)(const char* pattern_str, int32_t offset);
};
extern struct pattern_t pattern;