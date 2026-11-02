#pragma once
#include <algorithm>
#include <cmath>

namespace GodfatherCamera
{
    inline float Horizontal(float authored, float aspect, float factor, bool gameplay)
    {
        constexpr float originalAspect = 4.0f / 3.0f;
        constexpr float originalAngle = 1.0471976f;
        const float maximumAngle = gameplay ? 2.61799388f : 3.14059265f;
        if (!std::isfinite(authored) || authored <= 0.0f)
            authored = originalAngle;
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            aspect = originalAspect;
        if (!std::isfinite(factor))
            factor = 1.0f;
        const float horizontal = 2.0f * std::atan(std::tan(std::min(authored, 3.14059265f) * 0.5f)
            * aspect / originalAspect);
        return std::clamp(horizontal * (gameplay ? std::clamp(factor, 0.5f, 2.0f) : 1.0f), 0.001f, maximumAngle);
    }
}
