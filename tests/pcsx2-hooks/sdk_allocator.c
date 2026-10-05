/* Compile as Win32: its pointer/header sizes match the actual EE allocator. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>

#define malloc sdk_malloc
#define calloc sdk_calloc
#define realloc sdk_realloc
#define free sdk_free
#define _malloc_r sdk_malloc_r
#define _calloc_r sdk_calloc_r
#define _realloc_r sdk_realloc_r
#define _free_r sdk_free_r
#define _exit sdk_exit
#define __attribute__(x)
#define __builtin_trap() abort()
#define __builtin_unreachable() __assume(0)
#define PCSX2F_MODULE(entry, stack, bytes) void (*sdk_registered_entry)(const void*) = entry
void init(void) {}
void (*__init_array_start[1])(void);
void (*__init_array_end[1])(void);
#include "../../external/ps2sdk/plugins/module-runtime.c"
#include "../../external/injector/include/ps2/memalloc.c"

__declspec(align(16)) static uint8_t storage[1024 * 1024 + 32];
int main(void)
{
    const size_t budget = 1024 * 1024;
    const size_t corona_bytes = 1024 * 112;
    void* allocations[512] = {0};
    unsigned count = 0;
    assert(sizeof(Block) == 16);
    assert(sizeof(struct mem_block) <= 16);
    memset(storage, 0xa5, sizeof(storage));
    heap = (Block*)(storage + 16); heap_end = storage + 16 + budget;
    heap->size = budget - sizeof(Block); heap->next = NULL; heap->free = 1;
    uint8_t* coronas = AllocMemBlock(corona_bytes);
    assert(coronas && !((uintptr_t)coronas & 15));
    memset(coronas, 0x59, corona_bytes);
    for (unsigned i = 0; i < 100; ++i) {
        allocations[count] = AllocMemBlock(128 + i * 4);
        assert(allocations[count] && !((uintptr_t)allocations[count] & 15));
        ++count;
    }
    for (unsigned i = 0; i < count; i += 2) {
        FreeMemBlock(allocations[i]); allocations[i] = NULL;
    }
    for (;;) {
        void* p = AllocMemBlock(4096);
        if (!p) break;
        assert(count < 512);
        allocations[count++] = p;
        assert((uint8_t*)p >= storage + 16 && (uint8_t*)p + 4096 <= heap_end);
    }
    assert(!AllocMemBlock(SIZE_MAX));
    for (size_t i = 0; i < corona_bytes; ++i) assert(coronas[i] == 0x59);
    for (unsigned i = 0; i < 16; ++i) assert(storage[i] == 0xa5 && storage[16 + budget + i] == 0xa5);
    FreeMemBlock(coronas + 1); /* Cannot free an interior pointer. */
    for (unsigned i = 0; i < count; ++i) FreeMemBlock(allocations[i]);
    FreeMemBlock(coronas); FreeMemBlock(coronas);
    assert(!mem_arenas);
    assert(heap->free && !heap->next && heap->size == budget - sizeof(Block));
    void* whole_heap = sdk_malloc(budget - sizeof(Block));
    assert(whole_heap && !sdk_malloc(1));
    sdk_free(whole_heap);
    puts("Private SDK heap: 114688-byte corona array, fragmented hooks, exhaustion, exact budget, and complete recovery passed.");
    return 0;
}
