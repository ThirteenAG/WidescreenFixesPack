#include "../../external/injector/include/ps2/hooks.h"
#include <stdlib.h>
#include <assert.h>

#define EXPORT __declspec(dllexport)
static uint32_t memory[64];
static unsigned releases, writes, flushes;
static int fail_write, fail_alloc;
static int read_memory(void* user, uint32_t address, void* out, size_t size)
{
    (void)user;
    if (address < 0x1000 || address + size > 0x1000 + sizeof(memory)) return 0;
    memcpy(out, (char*)memory + address - 0x1000, size); return 1;
}
static int write_memory(void* user, uint32_t address, const void* in, size_t size)
{
    (void)user;
    if (fail_write || address < 0x1000 || address + size > 0x1000 + sizeof(memory)) return 0;
    ++writes; memcpy((char*)memory + address - 0x1000, in, size); return 1;
}
static int allocate_code(void* user, size_t bytes, uint32_t near_address, pcsx2_hook_buffer* out)
{
    uintptr_t aligned;
    (void)user; (void)near_address;
    if (fail_alloc) return 0;
    out->allocation = malloc(bytes + 15);
    if (!out->allocation) return 0;
    aligned = ((uintptr_t)out->allocation + 15) & ~(uintptr_t)15;
    out->words = (uint32_t*)aligned; out->capacity = bytes; out->address = 0x02000000;
    return 1;
}
static void release_code(void* user, pcsx2_hook_buffer* code)
{ (void)user; ++releases; free(code->allocation); }
static void flush_code(void* user, uint32_t address, size_t size)
{ (void)user; (void)address; assert(size != 0); ++flushes; }
static pcsx2_hook_backend backend = { NULL, read_memory, write_memory, allocate_code, release_code, flush_code, 0x02000060 };

EXPORT int relocate(const uint32_t* source, unsigned count, uint32_t pc,
    uint32_t* output, unsigned capacity, uint32_t new_pc, unsigned* written)
{
    size_t size = 0;
    int status = pcsx2_hook_relocate(source, count, pc, output, capacity, new_pc, &size);
    *written = (unsigned)size;
    return status;
}

EXPORT int mid_code(uint32_t* output, unsigned flags, unsigned* written, uint32_t* trampoline)
{
    pcsx2_hook hook = {0};
    int status;
    memset(memory, 0, sizeof(memory));
    status = pcsx2_hook_create_mid(&hook, &backend, 0x1010, 0x03000000, 0x12345678,
                                 0x024abc00, flags, 2);
    if (status == PCSX2_HOOK_OK) {
        *written = (unsigned)(hook.code_size / 4); *trampoline = hook.trampoline;
        memcpy(output, hook.code.words, hook.code_size);
        pcsx2_hook_destroy(&hook);
    }
    return status;
}

EXPORT int lifecycle(void)
{
    pcsx2_hook hook = {0};
    uint32_t source[] = {0x27bdffe0, 0x7fbf0010, 0};
    memset(memory, 0, sizeof(memory)); memcpy(memory + 4, source, sizeof(source));
    releases = writes = flushes = 0; fail_write = fail_alloc = 0;
    assert(pcsx2_hook_create_inline(&hook, &backend, 0x1010, 0x03000000, 2) == PCSX2_HOOK_OK);
    assert(writes == 0 && hook.trampoline == 0x02000000);
    assert(pcsx2_hook_enable(&hook) == PCSX2_HOOK_OK && writes == 1 && flushes == 2);
    assert(pcsx2_hook_enable(&hook) == PCSX2_HOOK_OK && writes == 1);
    memory[4] = 42;
    assert(pcsx2_hook_disable(&hook) == PCSX2_HOOK_CONFLICT);
    assert(pcsx2_hook_destroy(&hook) == PCSX2_HOOK_CONFLICT && releases == 0);
    memory[4] = hook.patch[0]; fail_write = 1;
    assert(pcsx2_hook_destroy(&hook) == PCSX2_HOOK_WRITE_FAILED && releases == 0);
    fail_write = 0;
    assert(pcsx2_hook_disable(&hook) == PCSX2_HOOK_OK);
    assert(memcmp(memory + 4, source, 8) == 0);
    assert(pcsx2_hook_destroy(&hook) == PCSX2_HOOK_OK && releases == 1);
    fail_alloc = 1;
    assert(pcsx2_hook_create_inline(&hook, &backend, 0x1010, 0x03000000, 2) == PCSX2_HOOK_ALLOCATION_FAILED);
    assert(memcmp(memory + 4, source, 8) == 0);
    fail_alloc = 0;
    memory[4] = 0x0c000800; /* JAL cannot be silently copied. */
    assert(pcsx2_hook_create_inline(&hook, &backend, 0x1010, 0x03000000, 2) == PCSX2_HOOK_UNSUPPORTED_INSTRUCTION);
    assert(releases == 1);
    memory[4] = 0; memory[5] = 0x10000001;
    assert(pcsx2_hook_create_inline(&hook, &backend, 0x1010, 0x03000000, 2) == PCSX2_HOOK_OK);
    assert(hook.count == 3); /* Automatically include the branch's delay slot. */
    assert(pcsx2_hook_destroy(&hook) == PCSX2_HOOK_OK);
    assert(sizeof(pcsx2_hook_context) == 688);
    assert(offsetof(pcsx2_hook_context, hi) == 512);
    assert(offsetof(pcsx2_hook_context, sa) == 544);
    assert(offsetof(pcsx2_hook_context, fpr) == 552);
    return 1;
}
