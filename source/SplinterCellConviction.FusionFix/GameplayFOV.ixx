module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module GameplayFOV;

export namespace GameplayFOV
{
    inline float Scale(float angle, float factor, float aspect)
    {
        if (!std::isfinite(angle) || angle <= 0.0f || angle >= 180.0f)
            return angle;
        factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return angle;
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            aspect = 16.0f / 9.0f;
        constexpr float radians = 3.14159265358979323846f / 180.0f;
        // The engine expands this 16:9 horizontal angle for the final viewport.
        const float limit = 2.0f * std::atan(std::tan(75.0f * radians) * (16.0f / 9.0f) / aspect) / radians;
        return std::clamp(angle * factor, 1.0f, std::max(1.0f, limit));
    }
}
