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

    inline Frame Fit(float width, float height, float videoWidth, float videoHeight, int mode)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(videoWidth) || !std::isfinite(videoHeight) ||
            width <= 0 || height <= 0 || videoWidth <= 0 || videoHeight <= 0)
            return {};
        const float areaWidth = mode == 0 ? std::min(width, height * 4.0f / 3.0f) : width;
        const float areaHeight = mode == 0 ? std::min(height, width * 3.0f / 4.0f) : height;
        const float scale = std::min(areaWidth / videoWidth, areaHeight / videoHeight);
        const float fittedWidth = videoWidth * scale, fittedHeight = videoHeight * scale;
        return { (width - fittedWidth) * 0.5f, (height - fittedHeight) * 0.5f, fittedWidth, fittedHeight };
    }
}
