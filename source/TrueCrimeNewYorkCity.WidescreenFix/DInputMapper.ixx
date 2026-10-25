module;

#include <stdafx.h>
#include "DInputActivation.hpp"

export module DInputMapper;

SafetyHookInline configureHook;
int32_t __fastcall ConfigureDevices(void* self, void*, HWND parent, void* target, void* callback, DWORD flags, void* reference)
{
    TCDInputActivation::Scope activation;
    return configureHook.unsafe_thiscall<int32_t>(self, parent, target, callback, flags, reference);
}

export void InitDInputMapper()
{
    // The native wrapper calls IDirectInput8::ConfigureDevices and returns
    // through one RET20 path. Scope COM redirection to that call only.
    auto pattern = hook::pattern("83 EC 44 53 55 8B 2D ? ? ? ? 56 57 8B F1 33 FF 33 DB 39 7E 14 76 20");
    if (pattern.size() != 1)
    {
        OutputDebugStringA("TCNYC: controller mapping wrapper pattern unavailable.\n");
        return;
    }
    configureHook = safetyhook::create_inline(pattern.get_first(), ConfigureDevices);
    if (!configureHook) OutputDebugStringA("TCNYC: controller mapping wrapper hook unavailable.\n");
}
