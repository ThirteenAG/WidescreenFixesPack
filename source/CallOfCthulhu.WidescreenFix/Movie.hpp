#pragma once
#include <algorithm>
#include <cmath>

namespace CoCMovie
{
    struct Rectangle
    {
        int left = 0, top = 0, width = 0, height = 0;
    };

    inline bool Frame(Rectangle& rectangle, int width, int height, int movieWidth, int movieHeight, int mode)
    {
        if (width <= 0 || height <= 0 || width > 16384 || height > 16384 ||
            movieWidth <= 0 || movieHeight <= 0 || movieWidth > 16384 || movieHeight > 16384 || mode < 0 || mode > 2)
            return false;
        double drawWidth = width;
        double drawHeight = height;
        if (mode != 2)
        {
            const double aspect = double(movieWidth) / movieHeight;
            if ((double(width) / height > aspect) == (mode == 0))
                drawWidth = height * aspect;
            else
                drawHeight = width / aspect;
        }
        if (drawWidth < 1.0 || drawHeight < 1.0 || drawWidth > 1048576.0 || drawHeight > 1048576.0)
            return false;
        rectangle.width = int(std::lround(drawWidth));
        rectangle.height = int(std::lround(drawHeight));
        rectangle.left = (width - rectangle.width) / 2;
        rectangle.top = (height - rectangle.height) / 2;
        return true;
    }

    struct Metadata
    {
        void* texture = nullptr;
        int width = 0, height = 0;
    };

    inline Metadata FromDecoder(const void* decoder, void* texture, int height)
    {
        auto format = decoder ? *static_cast<const unsigned char* const*>(decoder) : nullptr;
        const int width = format ? *reinterpret_cast<const int*>(format + 352) : 0;
        if (!texture || width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return {};
        return { texture, width, height };
    }

    inline Metadata frame;

    inline Metadata Take(void* texture)
    {
        auto published = frame;
        frame = {};
        return texture && published.texture == texture ? published : Metadata{};
    }
}
