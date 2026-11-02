#pragma once
#include <algorithm>
#include <cmath>

namespace GodfatherMovies
{
    struct Frame
    {
        float left = 0.0f, top = 0.0f, right = 0.0f, bottom = 0.0f;
        float u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;
    };

    // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole rectangle
    inline Frame Fit(float left, float top, float right, float bottom, float sourceAspect, int mode)
    {
        if (!std::isfinite(left) || !std::isfinite(top) || !std::isfinite(right) || !std::isfinite(bottom)
            || !std::isfinite(sourceAspect) || sourceAspect <= 0.0f || right <= left || bottom <= top)
            return {};
        if (mode == 0)
        {
            const float areaWidth = std::min(right - left, (bottom - top) * 4.0f / 3.0f);
            const float areaHeight = std::min(bottom - top, (right - left) * 3.0f / 4.0f);
            left += (right - left - areaWidth) * 0.5f;
            top += (bottom - top - areaHeight) * 0.5f;
            right = left + areaWidth;
            bottom = top + areaHeight;
        }
        Frame frame{ left, top, right, bottom };
        const float width = right - left, height = bottom - top;
        const float aspect = width / height;
        if (sourceAspect > aspect)
        {
            const float h = width / sourceAspect;
            frame.top += (height - h) * 0.5f;
            frame.bottom = frame.top + h;
        }
        else
        {
            const float w = height * sourceAspect;
            frame.left += (width - w) * 0.5f;
            frame.right = frame.left + w;
        }
        return frame;
    }
}
