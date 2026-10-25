#pragma once
#include <algorithm>
#include <cmath>

namespace Rev2CameraFOV
{
    inline float Scale(float fov, float factor)
    {
        if (!std::isfinite(fov) || fov <= 0.0f || fov >= 180.0f ||
            !std::isfinite(factor) || factor <= 0.0f || factor == 1.0f)
            return fov;
        auto result = std::min(fov * std::clamp(factor, 0.5f, 2.0f), std::max(fov, 170.0f));
        return std::isfinite(result) && result > 0.0f ? result : fov;
    }
}
