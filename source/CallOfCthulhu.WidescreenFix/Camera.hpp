#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace CoCCamera
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

    inline float Apply(const void* state, float verticalFOV, float factor)
    {
        if (!BaseGameplay(state) || !std::isfinite(verticalFOV) || verticalFOV <= 0.0f || verticalFOV >= 179.0f)
            return verticalFOV;
        if (!std::isfinite(factor))
            factor = 1.0f;
        return std::clamp(verticalFOV * std::clamp(factor, 0.5f, 2.0f), 1.0f, 179.0f);
    }
}
