#pragma once
#include <algorithm>
#include <cmath>

namespace SaboteurCamera
{
    struct Projection
    {
        float Width;
        float Height;
    };

    inline Projection Cinematic(float fieldOfView, float inverseAspect, int borders)
    {
        constexpr double pi = 3.14159265358979323846;
        constexpr double referenceAspect = 16.0 / 9.0;
        if (!std::isfinite(inverseAspect) || inverseAspect <= 0.0f)
            inverseAspect = 1.0f / static_cast<float>(referenceAspect);
        if (!std::isfinite(fieldOfView) || fieldOfView <= 0.0f || fieldOfView >= pi)
            fieldOfView = 1.0471976f;
        const double relativeAspect = 1.0 / (inverseAspect * referenceAspect);
        const bool contained = relativeAspect < 1.0 && (borders & 1);
        const double width = std::min(std::tan(static_cast<double>(fieldOfView) * 0.5) *
            (contained ? 1.0 : relativeAspect), std::tan(75.0 * pi / 180.0));
        return { static_cast<float>(width), static_cast<float>(width * inverseAspect) };
    }

    inline Projection Widescreen(float fieldOfView, float inverseAspect, float factor = 1.0f, bool widescreen = true)
    {
        constexpr double pi = 3.14159265358979323846;
        if (!std::isfinite(inverseAspect) || inverseAspect <= 0.0f)
            inverseAspect = 3.0f / 4.0f;
        if (!std::isfinite(fieldOfView) || fieldOfView <= 0.0f || fieldOfView >= pi)
            fieldOfView = 1.0471976f;

        const double correction = widescreen ? 0.75 / inverseAspect : 1.0;
        if (!std::isfinite(factor))
            factor = 1.0f;
        const double nativeWidth = std::tan(static_cast<double>(fieldOfView) * 0.5);
        const double widened = std::atan(nativeWidth * correction) * 2.0;
        const double angle = std::min(widened * std::clamp(static_cast<double>(factor), 0.5, 2.0), 150.0 * pi / 180.0);
        const double width = std::tan(angle * 0.5);
        return { static_cast<float>(width), static_cast<float>(width * inverseAspect) };
    }
}
