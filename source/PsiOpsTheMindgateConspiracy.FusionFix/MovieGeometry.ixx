module;
#include <stdafx.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

export module MovieGeometry;


export namespace PsiOpsMovies
{
    struct Rectangle
    {
        float x = 0.0f, y = 0.0f, width = 0.0f, height = 0.0f;
    };

    // Both modes show the whole movie at its own aspect
    inline Rectangle Fit(int width, int height, float aspect, int)
    {
        if (width <= 0 || height <= 0 || !std::isfinite(aspect) || aspect <= 0.0f)
            return {};
        const auto scale = std::min(float(width) / aspect, float(height));
        Rectangle rectangle{ 0.0f, 0.0f, scale * aspect, scale };
        rectangle.x = (float(width) - rectangle.width) * 0.5f;
        rectangle.y = (float(height) - rectangle.height) * 0.5f;
        return rectangle;
    }

    struct Affine
    {
        float xx = 1.0f, xy = 0.0f, yx = 0.0f, yy = 1.0f, x = 0.0f, y = 0.0f;

        bool Inverse(float screenX, float screenY, float& resultX, float& resultY) const
        {
            const auto determinant = xx * yy - xy * yx;
            if (!std::isfinite(determinant) || std::abs(determinant) < 0.000001f)
                return false;
            screenX -= x;
            screenY -= y;
            resultX = (screenX * yy - screenY * xy) / determinant;
            resultY = (screenY * xx - screenX * yx) / determinant;
            return std::isfinite(resultX) && std::isfinite(resultY);
        }
    };

    inline bool Reframe(uint8_t* vertices, size_t count, size_t stride, const Affine& projection, int width, int height, const Rectangle& destination)
    {
        if (!vertices || (count != 4 && count != 6) || stride < 12 || stride > 64 || width <= 0 || height <= 0 || destination.width <= 0.0f || destination.height <= 0.0f)
            return false;
        float screen[6][2]{};
        float left = 1.0e10f, top = left, right = -left, bottom = right;
        for (size_t i = 0; i < count; ++i)
        {
            float point[2];
            std::memcpy(point, vertices + i * stride, sizeof(point));
            screen[i][0] = point[0] * projection.xx + point[1] * projection.xy + projection.x;
            screen[i][1] = point[0] * projection.yx + point[1] * projection.yy + projection.y;
            if (!std::isfinite(screen[i][0]) || !std::isfinite(screen[i][1]))
                return false;
            left = std::min(left, screen[i][0]);
            right = std::max(right, screen[i][0]);
            top = std::min(top, screen[i][1]);
            bottom = std::max(bottom, screen[i][1]);
        }
        if (right - left < width * 0.2f || bottom - top < height * 0.2f)
            return false;
        unsigned corners = 0;
        for (size_t i = 0; i < count; ++i)
        {
            const bool onLeft = std::abs(screen[i][0] - left) <= 0.75f;
            const bool onRight = std::abs(screen[i][0] - right) <= 0.75f;
            const bool onTop = std::abs(screen[i][1] - top) <= 0.75f;
            const bool onBottom = std::abs(screen[i][1] - bottom) <= 0.75f;
            if ((!onLeft && !onRight) || (!onTop && !onBottom))
                return false;
            corners |= 1u << (unsigned(onRight) + 2u * unsigned(onBottom));
        }
        if (corners != 15)
            return false;
        float points[6][2]{};
        for (size_t i = 0; i < count; ++i)
        {
            const auto x = destination.x - 0.5f + (screen[i][0] - left) / (right - left) * destination.width;
            const auto y = destination.y - 0.5f + (screen[i][1] - top) / (bottom - top) * destination.height;
            if (!projection.Inverse(x, y, points[i][0], points[i][1]))
                return false;
        }
        for (size_t i = 0; i < count; ++i)
            std::memcpy(vertices + i * stride, points[i], sizeof(points[i]));
        return true;
    }
}
