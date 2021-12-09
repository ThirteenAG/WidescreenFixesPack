#pragma once
#include <stddef.h>
#include <stdint.h>

struct pattern_t {
    uintptr_t base_addr;
    uintptr_t(*get_first)(uintptr_t range_start, size_t range_size, char* pattern_str, int32_t offset);
};
extern struct pattern_t pattern;