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

    // The whole movie at its own aspect, centred
    inline Scale Layout(float width, float height, int videoWidth, int videoHeight)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f ||
            videoWidth <= 0 || videoHeight <= 0 || videoWidth > 16384 || videoHeight > 16384)
            return {};
        const auto horizontal = width / videoWidth;
        const auto vertical = height / videoHeight;
        const auto scale = std::min(horizontal, vertical);
        return { videoWidth * scale / width, videoHeight * scale / height };
    }
}
