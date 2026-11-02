module;

#include <stdafx.h>
#include <algorithm>

export module MovieFrame;

export namespace SCMovie
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

        const float areaWidth = mode == 0 ? std::min(float(width), float(height) * 4.0f / 3.0f) : float(width);
        const float areaHeight = mode == 0 ? std::min(float(height), float(width) * 3.0f / 4.0f) : float(height);

        // Most cinematics hold a 16:9 picture between black bars,
        // but others use the whole 4:3 frame or a taller picture, so the bars can't be removed by a fixed amount.
        const float aspect = float(videoWidth) / float(videoHeight);
        frame.width = std::min(areaWidth, areaHeight * aspect);
        frame.height = std::min(areaHeight, areaWidth / aspect);
        frame.x = (float(width) - frame.width) * 0.5f;
        frame.y = (float(height) - frame.height) * 0.5f;
        return frame;
    }
}
