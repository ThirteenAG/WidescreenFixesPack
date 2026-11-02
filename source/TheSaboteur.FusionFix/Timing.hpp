#pragma once
#include <cmath>

namespace SaboteurTiming
{
    inline double Damping(double original, float frameTime)
    {
        if (!std::isfinite(frameTime) || frameTime < 0.0f)
            return 1.0;
        return std::pow(original, static_cast<double>(frameTime) * 60.0);
    }

    inline double MouseScale(float frameTime)
    {
        if (!std::isfinite(frameTime) || frameTime <= 0.0f)
            return 0.04;
        return 0.04 / (60.0 * static_cast<double>(frameTime));
    }
}
