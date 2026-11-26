#pragma once
#include <cmath>

namespace scda
{
// The native lens supplies horizontal FOV in degrees for the 4:3 view.
inline float FovZoom(float degrees, float factor)
{
    if (factor == 1.0f || !(degrees > 0.0f && degrees < 180.0f)) return 1.0f;
    constexpr float pi = 3.1415927410125732f;
    const float half = (degrees * 0.5f) * pi / 180.0f;
    const float wider = half * factor;
    return std::tan(half) / std::tan(wider < 1.48f ? wider : 1.48f);
}
}
