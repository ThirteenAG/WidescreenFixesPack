#pragma once
#include <algorithm>
#include <cmath>

namespace RevCameraFOV
{
    inline bool Scale(float& fov, float& nearClip, float farClip, float factor)
    {
        if (!std::isfinite(fov) || fov <= 0.0f || fov >= 180.0f ||
            !std::isfinite(nearClip) || nearClip <= 0.0f || !std::isfinite(farClip) || farClip <= nearClip ||
            !std::isfinite(factor) || factor <= 0.0f || factor == 1.0f)
            return false;
        auto adjusted = std::min(fov * std::clamp(factor, 0.5f, 2.0f), std::max(fov, 170.0f));
        auto adjustedNear = nearClip / (adjusted / fov);
        if (!std::isfinite(adjustedNear) || adjustedNear <= 0.0f || adjustedNear >= farClip)
            return false;
        nearClip = adjustedNear;
        fov = adjusted;
        return true;
    }
}
