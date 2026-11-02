module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Focus;

import ComVars;
import DeveloperOverlay;

namespace
{
    SafetyHookInline PausePredicate;
    uintptr_t(__cdecl* GetEngine)() = nullptr;

    BOOL __fastcall IsGameplayPaused(uintptr_t info, void* edx)
    {
        const auto paused = PausePredicate.fastcall<BOOL>(info, edx);
        if (paused || !info || !GetEngine || !WindowHandle || !IsWindow(WindowHandle) ||
            (!DeveloperOverlay::BlocksInput() && (GetForegroundWindow() == WindowHandle || !FixSettings().Bool("MAIN.PauseOnFocusLoss"))) ||
            (*reinterpret_cast<const uint8_t*>(info + 0x5F0) & 2))
            return paused;
        const auto engine = GetEngine();
        if (!engine)
            return paused;
        const auto vtable = *reinterpret_cast<uintptr_t**>(engine);
        const auto level = reinterpret_cast<uintptr_t(__thiscall*)(uintptr_t)>(vtable[0x114 / 4])(engine);
        if (!level || level != *reinterpret_cast<const uintptr_t*>(info + 0x148) ||
            *reinterpret_cast<const uintptr_t*>(level + 0x440))
            return paused;
        const auto actors = *reinterpret_cast<const uintptr_t* const*>(level + 0x28);
        const auto count = *reinterpret_cast<const int32_t*>(level + 0x2C);
        return actors && count > 0 && actors[0] == info ? TRUE : paused;
    }
}

export void InitFocus()
{
    auto pause = hook::pattern("55 8B EC 51 57 8B F9 F6 87 F0 05 00 00 02 74 07 33 C0 5F 8B E5 5D C3 8B 8F 48 01 00 00 E8 ? ? ? ? 85 C0 0F 84");
    auto engine = hook::pattern("83 3D ? ? ? ? 00 75 09 A1 ? ? ? ? 85 C0 75 05 A1 ? ? ? ? C3");
    GetEngine = reinterpret_cast<decltype(GetEngine)>(engine.get_first());
    PausePredicate = safetyhook::create_inline(pause.get_first(), IsGameplayPaused);
}
