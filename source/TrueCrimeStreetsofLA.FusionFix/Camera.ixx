module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>


export module Camera;


export namespace TrueCrimeCamera
{
    inline constexpr float OriginalAspect = 4.0f / 3.0f;
    inline constexpr float MaximumTangent = 3.7320508075688773f;

    inline float Frustum(float original, float horizontalScale, float aspect, float factor)
    {
        if (!std::isfinite(original) || original <= 0.0f || !std::isfinite(horizontalScale) ||
            horizontalScale <= 0.0f || !std::isfinite(aspect) || aspect <= 0.0f)
            return original;
        if (!std::isfinite(factor))
            factor = 1.0f;
        const double tangent = (aspect / OriginalAspect) * std::clamp(factor, 0.5f, 2.0f) /
            (static_cast<double>(original) * horizontalScale);
        return static_cast<float>(1.0 / (std::min(tangent, static_cast<double>(MaximumTangent)) * horizontalScale));
    }
}
