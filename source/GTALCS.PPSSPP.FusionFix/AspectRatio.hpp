#pragma once
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace lcsws
{
    inline bool ParseAspectRatio(const char* text, float& aspect)
    {
        if (!text)
            return false;

        char* end = nullptr;
        const float width = std::strtof(text, &end);
        if (end == text || !std::isfinite(width) || width <= 0.0f)
            return false;

        float height = 1.0f;
        if (*end == ':')
        {
            const auto start = end + 1;
            height = std::strtof(start, &end);
            if (end == start || !std::isfinite(height) || height <= 0.0f)
                return false;
        }
        if (*end)
            return false;

        const float ratio = width / height;
        if (!std::isfinite(ratio))
            return false;
        aspect = std::clamp(ratio, 0.5f, 8.0f);
        return true;
    }
}
