#include "gvm.h"

uintptr_t _get_addr_for_game_version(size_t argc, ...) {
    uintptr_t v[argc];
    va_list ap;
    va_start(ap, argc);
    for (int i = 0; i < argc; i++) {
        v[i] = va_arg(ap, int);
    }
    va_end(ap);
    return v[gvm.version];
}

void gvm_init(int version) {
    gvm.version = version;
}

struct gvm_t gvm =
{
    .version = 0,
    .init = gvm_init
};
