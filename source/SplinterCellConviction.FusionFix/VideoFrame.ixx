module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module VideoFrame;

export namespace SCVideoFrame
{
    struct Frame
    {
        float x = 0, y = 0, width = 0, height = 0;
    };

    // The whole movie at its own aspect, centred
    inline Frame Fit(float width, float height, float videoWidth, float videoHeight)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(videoWidth) || !std::isfinite(videoHeight) ||
            width <= 0 || height <= 0 || videoWidth <= 0 || videoHeight <= 0)
            return {};
        const float scale = std::min(width / videoWidth, height / videoHeight);
        const float fittedWidth = videoWidth * scale, fittedHeight = videoHeight * scale;
        return { (width - fittedWidth) * 0.5f, (height - fittedHeight) * 0.5f, fittedWidth, fittedHeight };
    }
}
