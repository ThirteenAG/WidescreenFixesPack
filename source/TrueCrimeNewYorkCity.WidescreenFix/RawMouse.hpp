#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace TCRawMouse
{
    inline std::atomic<float> sensitivity{ 1.0f };

    inline void SetSensitivity(float value)
    {
        sensitivity = std::isfinite(value) ? std::clamp(value, 0.1f, 5.0f) : 1.0f;
    }

    inline float Scale(float nativeSensitivity)
    {
        return std::isfinite(nativeSensitivity) ? nativeSensitivity * sensitivity.load() / 1000.0f : 0.0f;
    }

    struct Delta
    {
        float x, y;
    };
    inline Delta FromCounts(int32_t x, int32_t y, float nativeSensitivity)
    {
        auto scale = Scale(nativeSensitivity);
        return { static_cast<float>(x) * scale, -static_cast<float>(y) * scale };
    }
}
