#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace WaterFrustum
{
    inline double Cross(const RwV2d& a, const RwV2d& b, const RwV2d& c)
    {
        return (double(b.x) - a.x) * (double(c.y) - a.y)
             - (double(b.y) - a.y) * (double(c.x) - a.x);
    }

    struct Polygon
    {
        std::array<RwV2d, 5> points{};
        int count{};
    };

    inline Polygon ConvexHull(std::array<RwV2d, 5> points)
    {
        for (const auto& point : points)
            if (!std::isfinite(point.x) || !std::isfinite(point.y))
                return {};

        std::sort(points.begin(), points.end(), [](const auto& a, const auto& b)
        {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        });
        const auto end{ std::unique(points.begin(), points.end(), [](const auto& a, const auto& b)
        {
            return a.x == b.x && a.y == b.y;
        }) };
        const int size{ static_cast<int>(end - points.begin()) };
        if (size < 3)
            return {};

        std::array<RwV2d, 10> hull{};
        int count{};
        for (int i{}; i < size; ++i)
        {
            while (count >= 2 && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        const int lowerCount{ count };
        for (int i{ size - 2 }; i >= 0; --i)
        {
            while (count > lowerCount && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        --count; // the first point is repeated at the end
        if (count < 3)
            return {};

        Polygon result{};
        result.count = count;
        std::copy_n(hull.begin(), count, result.points.begin());
        return result;
    }

    inline bool Intersects(const Polygon& polygon, int x, int y)
    {
        for (int i{}; i < polygon.count; ++i)
        {
            const auto& a{ polygon.points[i] };
            const auto& b{ polygon.points[(i + 1) % polygon.count] };
            const RwV2d corner{ float(x + (b.y < a.y)), float(y + (b.x > a.x)) };
            if (Cross(a, b, corner) < -0.000001)
                return false;
        }
        return true;
    }
}
