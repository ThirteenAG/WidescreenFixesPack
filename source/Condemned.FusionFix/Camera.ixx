module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>

export module Camera;


export namespace CondemnedCamera
{
    inline float GameplayFov(float vertical, float factor, unsigned width, unsigned height, float aspectScale)
    {
        if (!std::isfinite(vertical) || vertical <= 0.0f || !width || !height ||
            !std::isfinite(aspectScale) || aspectScale <= 0.0f)
            return vertical;

        constexpr double maximumHorizontal = 150.0 * 3.14159265358979323846 / 180.0;
        const double aspect = static_cast<double>(width) / height;
        const double multiplier = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        // The native camera scales horizontal FOV after converting from vertical.
        const double halfHorizontal = maximumHorizontal / (2.0 * aspectScale);
        if (halfHorizontal >= 1.5707963267948966)
            return static_cast<float>(vertical * multiplier);
        const double maximumVertical = 2.0 * std::atan(std::tan(halfHorizontal) / aspect);
        return static_cast<float>((std::min)(vertical * multiplier, maximumVertical));
    }
}
