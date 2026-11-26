module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Graphics;

// MAIN.ColorFilter (live): the yellow/orange grade of the levels. Every frame the area's light settings are copied into
// the colour correction filter, a list of correctors (enable byte, type, two colour vectors, 0x30 bytes each). The
// second one is the area's colour grade; with the setting off it stays disabled. The game's own Special Settings
// filters (classic horror, retro, noise) use other correctors and keep working.

namespace
{
    constexpr uint32_t GradeEnable = 0x64; // second corrector's enable byte in the filter

    SafetyHookInline CopyCorrectorsHook;
    void __fastcall CopyCorrectors(uintptr_t lightSettings, void* edx, uintptr_t filter)
    {
        static auto& colorFilter = FixSettings().Int("MAIN.ColorFilter");
        CopyCorrectorsHook.unsafe_thiscall(lightSettings, filter);
        if (filter && !colorFilter)
            *reinterpret_cast<uint8_t*>(filter + GradeEnable) = 0;
    }
}

export namespace Graphics
{
    void Init()
    {
        auto pattern = hook::pattern("55 8B EC 83 E4 F0 8B 45 08 83 EC 14 85 C0 53 56 57 8B D1 0F 84 ? ? ? ? 80 7A 14 00");
        CopyCorrectorsHook = safetyhook::create_inline(pattern.get_first(), CopyCorrectors);

        // GRAPHICS.ShadowResolution: the shadow map size the game picks for its High shadow quality (1024); Low and
        // Medium keep their sizes
        static const int shadowSize = std::min(FixSettings().Int("GRAPHICS.ShadowResolution"), 8192);
        if (shadowSize > 1024)
        {
            pattern = hook::pattern("8B 44 24 08 83 C0 ? 56 8B F1 83 E0");
            static auto ShadowSizeHook = safetyhook::create_mid(pattern.get_first(4), [](SafetyHookContext& regs)
            {
                if (static_cast<int>(regs.eax) >= 1024)
                    regs.eax = static_cast<uint32_t>(shadowSize);
            });
        }
    }
}
