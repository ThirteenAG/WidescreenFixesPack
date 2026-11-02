module;
#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module CameraMath;


export namespace PsiOpsCamera
{
    struct Framing
    {
        float x = 1.0f, y = 1.0f;
    };

    inline Framing Frame(int width, int height, int borders)
    {
        if (width <= 0 || height <= 0)
            return {};
        constexpr float original = 4.0f / 3.0f;
        const auto aspect = float(width) / float(height);
        return { borders & 2 ? std::max(1.0f, aspect / original) : 1.0f,
            borders & 1 ? std::max(1.0f, original / aspect) : 1.0f };
    }

    inline float BarHeight(float position, float offset, bool top)
    {
        if (!std::isfinite(position) || !std::isfinite(offset))
            return 0.0f;
        return std::clamp(top ? position + offset + 92.0f : 465.0f - position - offset, 0.0f, 232.5f);
    }

    inline float Multiply(float vertical, float gain, float factor)
    {
        if (!std::isfinite(vertical) || vertical <= 0.0f || !std::isfinite(gain) || gain <= 0.0f)
            return vertical;
        factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        constexpr float radians = 3.14159265358979323846f / 180.0f;
        const auto maximum = std::nextafter(2.0f * std::atan(std::tan(75.0f * radians) / gain), 0.0f);
        return std::min(vertical * factor, maximum);
    }
}
