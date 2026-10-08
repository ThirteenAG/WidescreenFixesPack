#pragma once
#include <algorithm>
#include <cmath>

// The desktop size and fitting a picture into a window.
namespace Desktop
{
    struct Rectangle { int left, top, right, bottom; };
    inline Rectangle Fit(int width, int height, int sourceWidth, int sourceHeight)
    {
        if (width <= 0 || height <= 0 || sourceWidth <= 0 || sourceHeight <= 0) return {};
        double scale = std::min(double(width) / sourceWidth, double(height) / sourceHeight);
        int w = std::clamp(int(std::lround(sourceWidth * scale)), 1, width);
        int h = std::clamp(int(std::lround(sourceHeight * scale)), 1, height);
        int x = (width - w) / 2, y = (height - h) / 2;
        return {x, y, x + w, y + h};
    }
    // A configured resolution: 0 on an axis takes the desktop size.
    inline void Resolution(int& x, int& y)
    {
        auto [desktopX, desktopY] = GetDesktopRes();
        if (x <= 0) x = desktopX;
        if (y <= 0) y = desktopY;
        x = std::clamp(x, 640, 7680);
        y = std::clamp(y, 480, 4320);
    }
}
