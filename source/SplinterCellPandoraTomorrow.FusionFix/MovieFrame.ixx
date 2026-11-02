module;

#include <stdafx.h>
#include <algorithm>

export module MovieFrame;

export namespace PTMovie
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

        // Native playback centers the decoded image inside a 4:3 texture area. Original fits that area, Fill fits the
        // decoded image itself (the area's bars are not part of the movie).
        const float encodedHeight = float(videoWidth) * 0.75f;
        const float scale = std::min(float(width) / float(videoWidth),
            float(height) / (mode != 0 ? std::min(float(videoHeight), encodedHeight) : encodedHeight));
        frame.width = float(videoWidth) * scale;
        frame.height = encodedHeight * scale;
        frame.x = (float(width) - frame.width) * 0.5f;
        frame.y = (float(height) - frame.height) * 0.5f;
        return frame;
    }
}
