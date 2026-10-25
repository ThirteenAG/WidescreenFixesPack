#pragma once

#include <algorithm>

namespace SCMovie
{
    struct Frame
    {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
        float left = 0.0f, top = 0.0f, right = 1.0f, bottom = 1.0f;
    };

    inline Frame Layout(int width, int height, int videoWidth, int videoHeight, int mode)
    {
        Frame frame;
        if (width <= 0 || height <= 0 || videoWidth <= 0 || videoHeight <= 0)
            return frame;

        mode = std::clamp(mode, 0, 2);
        frame.width = float(width);
        frame.height = float(height);
        if (mode == 2)
            return frame;

        const float screenAspect = float(width) / float(height);
        if (mode == 1)
        {
            // The original full-screen movies include a 16:9 image inside
            // their encoded frame. Remove those authored bars before filling.
            const int bars = std::max(0, (videoHeight - videoWidth * 9 / 16) / 2);
            frame.top = float(bars) / float(videoHeight);
            frame.bottom = 1.0f - frame.top;
            const float aspect = float(videoWidth) / float(videoHeight - 2 * bars);
            if (screenAspect > aspect)
            {
                const float crop = (frame.bottom - frame.top) * (1.0f - aspect / screenAspect) * 0.5f;
                frame.top += crop;
                frame.bottom -= crop;
            }
            else
            {
                frame.left = (1.0f - screenAspect / aspect) * 0.5f;
                frame.right = 1.0f - frame.left;
            }
            return frame;
        }

        const float aspect = float(videoWidth) / float(videoHeight);
        if (screenAspect > aspect)
        {
            frame.width = float(height) * aspect;
            frame.x = (float(width) - frame.width) * 0.5f;
        }
        else
        {
            frame.height = float(width) / aspect;
            frame.y = (float(height) - frame.height) * 0.5f;
        }
        return frame;
    }
}
