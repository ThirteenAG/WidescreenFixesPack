#define MEM_CUSTOM_TOTAL_SIZE 4096
#define mem_custom_initialized ps2_initialized
#define mem_custom ps2_pool
#define mem_freeList ps2_blocks
#define mem_initialize ps2_initialize
#define mem_split ps2_split
#define AllocMemBlock ps2_allocate
#define mem_merge ps2_merge
#define FreeMemBlock ps2_free
#include "../../external/injector/include/ps2/memalloc.c"
#undef mem_custom_initialized
#undef mem_custom
#undef mem_freeList
#undef mem_initialize
#undef mem_split
#undef AllocMemBlock
#undef mem_merge
#undef FreeMemBlock
#undef MEM_HEADER_SIZE
#define mem_custom_initialized psp_initialized
#define mem_custom psp_pool
#define mem_freeList psp_blocks
#define mem_initialize psp_initialize
#define mem_split psp_split
#define AllocMemBlock psp_allocate
#define mem_merge psp_merge
#define FreeMemBlock psp_free
#include "../../external/injector/include/psp/memalloc.c"

__declspec(dllexport) void pool_reset(int psp)
{ if (psp) { psp_initialized = 0; psp_initialize(); } else { ps2_initialized = 0; ps2_initialize(); } }
__declspec(dllexport) void* pool_allocate(int psp, size_t bytes)
{ return psp ? psp_allocate(bytes) : ps2_allocate(bytes); }
__declspec(dllexport) void pool_initialize(int psp)
{ if (psp) psp_initialize(); else ps2_initialize(); }
__declspec(dllexport) void pool_free(int psp, void* pointer)
{ if (psp) psp_free(pointer); else ps2_free(pointer); }
