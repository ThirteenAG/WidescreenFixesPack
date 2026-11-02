module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module MovieFrame;

export namespace MovieFrame
{
    struct Scale
    {
        float x = 1.0f, y = 1.0f;
    };

    inline Scale Layout(float width, float height, int videoWidth, int videoHeight, int mode)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f ||
            videoWidth <= 0 || videoHeight <= 0 || videoWidth > 16384 || videoHeight > 16384)
            return {};
        const float areaWidth = mode == 0 ? std::min(width, height * 4.0f / 3.0f) : width;
        const float areaHeight = mode == 0 ? std::min(height, width * 3.0f / 4.0f) : height;
        const auto horizontal = areaWidth / videoWidth;
        const auto vertical = areaHeight / videoHeight;
        const auto scale = std::min(horizontal, vertical);
        return { videoWidth * scale / width, videoHeight * scale / height };
    }
}
