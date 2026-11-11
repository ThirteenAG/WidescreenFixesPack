module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>

export module Camera;

import ComVars;



export namespace CoCCamera
{
    template<class T> T Read(const void* state, ptrdiff_t offset)
    {
        T value{};
        std::memcpy(&value, static_cast<const uint8_t*>(state) + offset, sizeof(value));
        return value;
    }

    inline bool BaseGameplay(const void* state)
    {
        if (!state || !Read<uint32_t>(state, -436) || Read<int32_t>(state, 220) != 0 ||
            Read<int32_t>(state, 0) != 0 || !Read<uint8_t>(state, 12))
            return false;
        for (int slot = 1; slot < 13; ++slot)
        {
            if (Read<uint8_t>(state, 12 * slot + 12))
                return false;
        }
        const float base = Read<float>(state, 4);
        const float current = Read<float>(state, 192);
        const float target = Read<float>(state, 196);
        // Preserve native blends back from aiming and scripted camera effects.
        return std::isfinite(base) && std::isfinite(current) && std::isfinite(target) &&
            std::abs(base - current) < 0.0001f && std::abs(base - target) < 0.0001f;
    }

    // The factor widens the view up to 150 degrees across the screen; a wider native view stays as it is
    inline float Apply(const void* state, float verticalFOV, float factor, float aspect)
    {
        if (!BaseGameplay(state) || !std::isfinite(verticalFOV) || verticalFOV <= 0.0f || verticalFOV >= 179.0f)
            return verticalFOV;
        if (!std::isfinite(factor))
            factor = 1.0f;
        constexpr float radians = 3.14159265f / 180.0f;
        const float widest = std::isfinite(aspect) && aspect > 0.0f ? 2.0f * std::atan(std::tan(75.0f * radians) / aspect) / radians : 179.0f;
        const float scaled = verticalFOV * std::clamp(factor, 0.5f, 2.0f);
        return std::clamp(std::min(scaled, std::max(verticalFOV, widest)), 1.0f, 179.0f);
    }
}

export namespace CoCCamera
{
    void Install()
    {
        // The player FOV priority stack reaches the camera setter before it builds
        // both the projection and culling planes. Keep its stored authored FOV intact.
        auto pattern = hook::pattern("8B 8E C0 00 00 00 51 8B C8 E8 ? ? ? ? 5E 83 C4 08 C3");
        static auto GameplayFOVHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
            auto argument = reinterpret_cast<float*>(regs.esp);
            *argument = CoCCamera::Apply(reinterpret_cast<void*>(regs.esi), *argument, factor, Screen.fAspectRatio);
        });
    }
}
