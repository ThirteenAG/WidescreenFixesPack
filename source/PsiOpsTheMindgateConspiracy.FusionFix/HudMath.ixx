module;
#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module HudMath;


export namespace PsiOpsHud
{
    struct Layout
    {
        float scaleX = 1.0f, scaleY = 1.0f;
        float horizontal = 0.0f, vertical = 0.0f;
    };

    inline Layout Calculate(int width, int height, float constraint)
    {
        if (width <= 0 || height <= 0)
            return {};
        constexpr float original = 4.0f / 3.0f;
        const auto aspect = float(width) / float(height);
        const auto safe = std::isfinite(constraint) && constraint > 0.0f ? std::clamp(constraint, original, 32.0f / 9.0f) : aspect;
        Layout result;
        result.scaleX = std::min(1.0f, original / aspect);
        result.scaleY = std::min(1.0f, aspect / original);
        result.horizontal = 256.0f * (std::max(1.0f, std::min(aspect, safe) / original) - 1.0f);
        result.vertical = 232.5f * (std::max(1.0f, original / aspect) - 1.0f);
        return result;
    }
}
