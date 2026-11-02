#pragma once
#include <algorithm>

namespace lcsws
{
    inline float CinematicAspect(float viewport, float original, float amount, int sides)
    {
        amount = std::clamp(amount, 0.0f, 1.0f);
        if (viewport > original)
        {
            const float visible = (sides & 2) ? 1.0f - amount * (1.0f - original / viewport) : 1.0f;
            return original / visible;
        }
        const float visible = (sides & 1) ? 1.0f - amount * (1.0f - viewport / original) : 1.0f;
        return viewport / visible;
    }
}
