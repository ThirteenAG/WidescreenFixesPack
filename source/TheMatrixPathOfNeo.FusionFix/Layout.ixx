module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>


export module Layout;


export namespace NeoLayout
{
    constexpr float DesignAspect = 4.0f / 3.0f;
    constexpr double Pi = 3.14159265358979323846;

    inline float ParseAspect(const std::string& text)
    {
        if (text.empty() || _stricmp(text.c_str(), "Auto") == 0)
            return 0.0f;
        char* end = nullptr;
        const double first = std::strtod(text.c_str(), &end);
        double value = first;
        if (end && (*end == ':' || *end == '/'))
        {
            char* last = nullptr;
            const double second = std::strtod(end + 1, &last);
            if (last == end + 1 || !last || *last || !std::isfinite(second) || second <= 0.0)
                return 0.0f;
            value /= second;
            end = last;
        }
        if (!end || end == text.c_str() || *end || !std::isfinite(value) || value <= 0.0)
            return 0.0f;
        return static_cast<float>(std::clamp(value, 4.0 / 3.0, 32.0 / 9.0));
    }

    inline float HalfVertical(float native, float aspect, float factor)
    {
        if (!std::isfinite(native) || native <= 0.0f || native >= 90.0f || !std::isfinite(aspect) || aspect <= 0.0f)
            return native;
        if (!std::isfinite(factor))
            factor = 1.0f;
        const double tangent = std::min(std::tan(native * Pi / 180.0) * std::clamp(factor, 0.5f, 2.0f),
            std::tan(75.0 * Pi / 180.0) / aspect);
        return static_cast<float>(std::atan(tangent) * 180.0 / Pi);
    }

    struct Transform
    {
        float ScaleX = 1.0f;
        float ScaleY = 1.0f;
        float OffsetX = 0.0f;

        float X(float value) const { return (value - 0.5f) * ScaleX + 0.5f + OffsetX; }
        float Y(float value) const { return (value - 0.5f) * ScaleY + 0.5f; }
    };

    inline Transform Box(float aspect, float constraint, int anchor)
    {
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            return {};
        const float box = constraint > 0.0f ? std::min(aspect, constraint) : aspect;
        const float scale = std::min(1.0f, aspect / DesignAspect);
        return { DesignAspect / aspect * scale, scale,
            static_cast<float>(anchor) * (box - DesignAspect * scale) / (2.0f * aspect) };
    }
}
