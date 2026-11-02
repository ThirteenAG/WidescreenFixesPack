module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module CinematicFrame;

export namespace SCCinematicFrame
{
    constexpr float Reference = 16.0f / 9.0f;

    struct Frame
    {
        float left = 0, top = 0, right = 0, bottom = 0;
    };

    inline Frame Fit(float width, float height, int borders)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || width <= 0 || height <= 0)
            return {};
        Frame frame{ 0, 0, width, height };
        if (width / height < Reference && (borders & 1))
        {
            frame.top = (height - width / Reference) * 0.5f;
            frame.bottom = height - frame.top;
        }
        else if (width / height > Reference && (borders & 2))
        {
            frame.left = (width - height * Reference) * 0.5f;
            frame.right = width - frame.left;
        }
        return frame;
    }

    inline float FieldOfView(float angle, float aspect, int borders)
    {
        if (!std::isfinite(angle) || angle <= 0 || angle >= 180 ||
            !std::isfinite(aspect) || aspect <= 0 || aspect >= Reference || !(borders & 1))
            return angle;
        constexpr float radians = 3.14159265358979323846f / 180.0f;
        return 2.0f * std::atan(std::tan(angle * radians * 0.5f) * Reference / aspect) / radians;
    }
}
