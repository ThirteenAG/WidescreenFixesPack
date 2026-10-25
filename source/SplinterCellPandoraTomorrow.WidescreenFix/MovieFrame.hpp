#pragma once

#include <algorithm>

namespace PTMovie
{
    struct Frame
    {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
    };

    inline Frame Layout(int width, int height, int videoWidth, int videoHeight, int mode)
    {
        Frame frame;
        if (width <= 0 || height <= 0 || videoWidth <= 0 || videoHeight <= 0 ||
            videoWidth > 16384 || videoHeight > 16384)
            return frame;

        mode = std::clamp(mode, 0, 2);
        frame.width = float(width);
        frame.height = float(height);
        // Native playback centers the decoded image inside a 4:3 texture area.
        const float encodedHeight = float(videoWidth) * 0.75f;
        if (mode == 2)
        {
            // Stretch the picture; keep unused texture padding outside the viewport.
            frame.height *= encodedHeight / std::min(float(videoHeight), encodedHeight);
            frame.y = (float(height) - frame.height) * 0.5f;
            return frame;
        }
        const float scale = mode == 1 ?
            std::max(float(width) / float(videoWidth), float(height) / std::min(float(videoHeight), encodedHeight)) :
            std::min(float(width) / float(videoWidth), float(height) / encodedHeight);
        frame.width = float(videoWidth) * scale;
        frame.height = encodedHeight * scale;
        frame.x = (float(width) - frame.width) * 0.5f;
        frame.y = (float(height) - frame.height) * 0.5f;
        return frame;
    }
}
