#pragma once
#include <algorithm>
#include <cmath>
#include <string_view>

namespace CameraFOV
{
    inline bool IsGameplayType(std::string_view name)
    {
        return name == "uCameraQFPS" || name == "uCameraVeh";
    }

    inline float Scale(float fov, float aspect, float factor)
    {
        if (!std::isfinite(fov) || fov <= 0.0f || !std::isfinite(aspect) || aspect <= 0.0f ||
            std::abs(aspect - 1.0f) <= 0.01f || !std::isfinite(factor) || factor <= 0.0f || factor == 1.0f)
            return fov;
        return (std::min)(fov * std::clamp(factor, 0.5f, 2.0f), 3.0f);
    }
}
