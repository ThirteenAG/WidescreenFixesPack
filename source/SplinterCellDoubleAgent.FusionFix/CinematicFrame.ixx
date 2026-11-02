module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module CinematicFrame;

export namespace DACinematicFrame
{
    struct Frame
    {
        float left = 0, top = 0, right = 0, bottom = 0, projectionAspect = 0;
    };
    inline Frame Fit(float width, float height, float reference, int borders)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(reference) ||
            width <= 0 || height <= 0 || reference <= 0)
            return {};
        Frame frame{ 0, 0, width, height, width / height };
        if (width / height < reference && (borders & 1))
        {
            frame.top = (height - width / reference) * 0.5f;
            frame.bottom = height - frame.top;
            frame.projectionAspect = reference;
        }
        else if (width / height > reference && (borders & 2))
        {
            frame.left = (width - height * reference) * 0.5f;
            frame.right = width - frame.left;
        }
        return frame;
    }
}
