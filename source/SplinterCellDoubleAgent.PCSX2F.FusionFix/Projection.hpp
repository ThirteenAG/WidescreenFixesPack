#pragma once
#include <cmath>

namespace scda {
// The camera's first projection row is cot(horizontal FOV / 2) for the 4:3
// frame. The factor widens that angle; the result scales the view uniformly.
inline float FovZoom(float cotangent, float factor) {
    if (factor == 1.0f || !(cotangent > 0.0f)) return 1.0f;
    const float half = std::atan(1.0f / cotangent);
    const float wider = half * factor;
    return (1.0f / cotangent) / std::tan(wider < 1.48f ? wider : 1.48f);
}
}
