#include <stdafx.h>
#include "NativeTextHooks.hpp"

namespace NeoText
{
    __declspec(naked) void Invoke(uintptr_t function, void* node, uintptr_t root, uintptr_t scene, uintptr_t ediValue, uintptr_t esiValue)
    {
        __asm
        {
            push ebp
            mov ebp, esp
            push ebx
            push edi
            push esi
            mov edi, [ebp + 24]
            mov esi, [ebp + 28]
            mov ecx, [ebp + 12]
            push [ebp + 20]
            push [ebp + 16]
            call [ebp + 8]
            pop esi
            pop edi
            pop ebx
            mov esp, ebp
            pop ebp
            ret
        }
    }
}
