#include "../../external/injector/include/psp/hooks.h"
#include <stdlib.h>
#include <assert.h>

static uint32_t psp_memory[64];
static int read_psp(void* user, uint32_t address, void* out, size_t size)
{
    (void)user;
    if (address < 0x1000 || address + size > 0x1000 + sizeof(psp_memory)) return 0;
    memcpy(out, (char*)psp_memory + address - 0x1000, size); return 1;
}
static int write_psp(void* user, uint32_t address, const void* in, size_t size)
{
    (void)user;
    if (address < 0x1000 || address + size > 0x1000 + sizeof(psp_memory)) return 0;
    memcpy((char*)psp_memory + address - 0x1000, in, size); return 1;
}
static int allocate_psp(void* user, size_t bytes, uint32_t near_address, psp_hook_buffer* out)
{
    (void)user; (void)near_address;
    out->allocation = malloc(bytes + 15);
    if (!out->allocation) return 0;
    out->words = (uint32_t*)(((uintptr_t)out->allocation + 15) & ~(uintptr_t)15);
    out->capacity = bytes; out->address = 0x02000000;
    return 1;
}
static void release_psp(void* user, psp_hook_buffer* code)
{ (void)user; free(code->allocation); }
static void flush_psp(void* user, uint32_t address, size_t size)
{ (void)user; (void)address; (void)size; }
static psp_hook_backend psp_backend = { NULL, read_psp, write_psp, allocate_psp, release_psp, flush_psp };

__declspec(dllexport) int psp_relocate(const uint32_t* source, unsigned count, uint32_t pc,
    uint32_t* output, unsigned capacity, uint32_t new_pc, unsigned* written)
{
    size_t size = 0;
    int status = psp_hook_relocate(source, count, pc, output, capacity, new_pc, &size);
    *written = (unsigned)size;
    return status;
}

__declspec(dllexport) int psp_mid_code(uint32_t* output, unsigned flags, unsigned* written)
{
    psp_hook hook = {0};
    int status;
    memset(psp_memory, 0, sizeof(psp_memory));
    status = psp_hook_create_mid(&hook, &psp_backend, 0x1010, 0x03000000, 0x12345678,
                               0x024abc00, flags, 2);
    if (status == PSP_HOOK_OK) {
        *written = (unsigned)(hook.code_size / 4);
        memcpy(output, hook.code.words, hook.code_size);
        psp_hook_destroy(&hook);
    }
    assert(offsetof(psp_hook_context, fpr) == 140);
    assert(offsetof(psp_hook_context, vfpu) == 272);
    assert(offsetof(psp_hook_context, vfpu_control) == 784);
    assert(sizeof(psp_hook_context) == 848);
    return status;
}
