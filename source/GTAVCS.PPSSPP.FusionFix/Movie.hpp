#pragma once
#include <algorithm>
#include <cmath>

namespace vcsws
{
struct MovieMap
{
    static constexpr int Width = 480, Height = 272;
    int x[Width]{}, y[Height]{};

    // Both modes show the whole movie at its own aspect with black bars
    void Set(float aspect, int)
    {
        constexpr float original = float(Width) / Height;
        if (!std::isfinite(aspect) || aspect <= 0.0f) aspect = original;
        const int width = std::max(1, int(std::lround(Width * std::min(1.0f, original / aspect))));
        const int height = std::max(1, int(std::lround(Height * std::min(1.0f, aspect / original))));
        const int left = (Width - width) / 2, top = (Height - height) / 2;
        for (int i = 0; i < Width; ++i)
            x[i] = i < left || i >= left + width ? -1 : (i - left) * Width / width;
        for (int i = 0; i < Height; ++i)
            y[i] = i < top || i >= top + height ? -1 : (i - top) * Height / height;
    }
};
}
