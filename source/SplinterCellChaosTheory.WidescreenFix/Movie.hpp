#pragma once
#include <algorithm>
#include <cmath>

namespace CTMovie
{
    struct Rectangle
    {
        float left, top, right, bottom;
        float u0, v0, u1, v1;
    };

    inline bool Frame(Rectangle& rectangle, float width, float height, float movieWidth, float movieHeight, int mode)
    {
        if (!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(movieWidth) || !std::isfinite(movieHeight) ||
            width <= 0.0f || height <= 0.0f || movieWidth <= 0.0f || movieHeight <= 0.0f)
            return false;

        rectangle.left = rectangle.top = 0.0f;
        rectangle.right = width;
        rectangle.bottom = height;
        float aspect = movieWidth / movieHeight;
        float screenAspect = width / height;
        if (mode == 0)
        {
            float drawWidth = std::min(width, height * aspect);
            float drawHeight = std::min(height, width / aspect);
            rectangle.left = (width - drawWidth) * 0.5f;
            rectangle.top = (height - drawHeight) * 0.5f;
            rectangle.right = rectangle.left + drawWidth;
            rectangle.bottom = rectangle.top + drawHeight;
        }
        else if (mode == 1)
        {
            float x = std::min(1.0f, screenAspect / aspect);
            float y = std::min(1.0f, aspect / screenAspect);
            float cropX = (rectangle.u1 - rectangle.u0) * (1.0f - x) * 0.5f;
            float cropY = (rectangle.v1 - rectangle.v0) * (1.0f - y) * 0.5f;
            rectangle.u0 += cropX;
            rectangle.u1 -= cropX;
            rectangle.v0 += cropY;
            rectangle.v1 -= cropY;
        }
        return true;
    }
}
