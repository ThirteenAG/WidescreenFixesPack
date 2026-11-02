module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module Movie;

export namespace CTMovie
{
    struct Rectangle
    {
        float left, top, right, bottom;
        float u0, v0, u1, v1;
    };

    // The whole movie at its own aspect: Original inside the game's centred 4:3 area, Fill inside the whole screen
    inline bool Frame(Rectangle& rectangle, float width, float height, float movieWidth, float movieHeight, int mode)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(movieWidth) || !std::isfinite(movieHeight) ||
            width <= 0.0f || height <= 0.0f || movieWidth <= 0.0f || movieHeight <= 0.0f)
            return false;

        rectangle.left = rectangle.top = 0.0f;
        rectangle.right = width;
        rectangle.bottom = height;
        float aspect = movieWidth / movieHeight;
        float areaWidth = mode == 0 ? std::min(width, height * 4.0f / 3.0f) : width;
        float areaHeight = mode == 0 ? std::min(height, width * 3.0f / 4.0f) : height;
        float drawWidth = std::min(areaWidth, areaHeight * aspect);
        float drawHeight = std::min(areaHeight, areaWidth / aspect);
        rectangle.left = (width - drawWidth) * 0.5f;
        rectangle.top = (height - drawHeight) * 0.5f;
        rectangle.right = rectangle.left + drawWidth;
        rectangle.bottom = rectangle.top + drawHeight;
        return true;
    }
}
