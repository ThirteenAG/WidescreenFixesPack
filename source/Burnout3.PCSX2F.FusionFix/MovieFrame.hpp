#pragma once
#include <algorithm>
#include <cmath>

namespace burnout
{
inline void FitMovieFrame(float* rect, int mode)
{
    if (!(rect[2] > 0.0f && rect[3] > 0.0f) || !std::isfinite(rect[0]) || !std::isfinite(rect[1]) ||
        !std::isfinite(rect[2]) || !std::isfinite(rect[3]) || std::fabs(rect[0] + rect[2] * 0.5f - 320.0f) > 1.0f ||
        std::fabs(rect[1] + rect[3] * 0.5f - 224.0f) > 1.0f)
        return;
    float scale = std::min(640.0f / rect[2], 448.0f / rect[3]);
    if (mode == 0)
        scale = std::min(scale, 1.0f);
    rect[2] *= scale;
    rect[3] *= scale;
    rect[0] = (640.0f - rect[2]) * 0.5f;
    rect[1] = (448.0f - rect[3]) * 0.5f;
}
} // namespace burnout
