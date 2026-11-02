module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module HudFrame;

export namespace HudFrame
{
    struct Offset
    {
        float x = 0.0f, y = 0.0f;
    };

    inline Offset Calculate(float aspect, float constraint, int horizontal, int vertical, bool split)
    {
        if (!std::isfinite(aspect) || aspect <= 0)
            return {};
        constexpr float reference = 16.0f / 9.0f;
        const float scale = std::min(1.0f, aspect / reference);
        const float stageWidth = split ? 640.0f : 1280.0f;
        const float nativeWidth = reference * scale / (split ? 2.0f : 1.0f);
        const float playerWidth = aspect / (split ? 2.0f : 1.0f);
        const float safeWidth = std::isfinite(constraint) && constraint > 0 ?
            std::min(playerWidth, std::clamp(constraint, 4.0f / 3.0f, 32.0f / 9.0f)) : playerWidth;
        return { horizontal * (safeWidth - nativeWidth) * 0.5f * stageWidth / nativeWidth,
            vertical * (1.0f - scale) * 360.0f / scale };
    }
}
