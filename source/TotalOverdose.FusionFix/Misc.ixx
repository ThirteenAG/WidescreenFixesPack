module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Misc;

import ComVars;

// The start-up script: the SCI and Deadline logos, the story intro, the "press Enter" title, then the front end loads.
// MAIN.SkipIntro skips the logos and continues with the story intro.
export void SkipIntro()
{
    auto startup = hook::pattern("8B 45 04 85 C0 5D 75 0D 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 8B 15 ? ? ? ? 52 57 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 8B 43 3C");
    auto chains = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 74 24 08 50 51 68 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE 5E E9 ? ? ? ? CC CC CC CC CC A1 ? ? ? ? 8B 0D ? ? ? ? 56 8B 74 24 08 50 51 68 ? ? ? ? 68 ? ? ? ? 8B CE E8 ? ? ? ? 8B CE 5E E9 ? ? ? ? CC CC CC CC CC 56 8B 74 24 08 8B 46 40 8B 40 08 83 E8 00 74 ? 48 74 ? 48 75 ? 8B 46 3C 85 C0 74 ? 6A 02");
    auto transition = startup.get_first<uint8_t>(21);
    const auto story = *chains.get_first<uint32_t>(67);
    injector::WriteMemory<uint32_t>(reinterpret_cast<uint32_t*>(transition + 9), story, true);
}

// MAIN.FOVFactor: the main view's vertical field of view (degrees) widened by the factor on its tangent, so 1.0 is
// the game's own camera; bounded to 150 degrees across. The other views (fixed 60 degrees) stay as they are.
SafetyHookInline SetProjectionHook;
int __fastcall SetProjection(void* renderer, void*, float fov, float aspect, float nearPlane, float farPlane, int view)
{
    static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
    if (view == -1 && std::isfinite(factor) && factor != 1.0f && std::isfinite(fov) && std::isfinite(aspect) && fov > 0.0f && fov < 180.0f && aspect > 0.0f)
    {
        constexpr float toRadians = 3.14159265f / 180.0f;
        const float maxVertical = 2.0f * std::atan(std::tan(75.0f * toRadians) / aspect);
        float vertical = 2.0f * std::atan(std::tan(fov * 0.5f * toRadians) * std::clamp(factor, 0.5f, 2.0f));
        vertical = std::min(vertical, std::max(maxVertical, fov * toRadians));
        fov = vertical / toRadians;
    }
    return SetProjectionHook.unsafe_thiscall<int>(renderer, fov, aspect, nearPlane, farPlane, view);
}

export void InstallFieldOfView()
{
    auto pattern = hook::pattern("8B 44 24 14 83 F8 FF 75 69 8B 41 1C 33 D2 85 C0 0F 8E");
    SetProjectionHook = safetyhook::create_inline(pattern.get_first(), SetProjection); //0x41FB20
}

