#pragma once

#include <algorithm>
#include <array>
#include <cmath>

// ScanWorld and RequestObjectsInFrustum project only one pair of corners onto
// the sector grid. When the camera pitches, the other pair can lie outside
// that triangle. Keep the native scanner, but give it the complete footprint.
namespace FrustumScan
{
    inline RwV2d ToSector(const RwV3d& point)
    {
        return { point.x * 0.02f + 48.0f, point.y * 0.02f + 40.0f };
    }

    inline RwV3d Transform(const RwV3d& point, const RwMatrix& matrix)
    {
        return {
            matrix.right.x * point.x + matrix.up.x * point.y + matrix.at.x * point.z + matrix.pos.x,
            matrix.right.y * point.x + matrix.up.y * point.y + matrix.at.y * point.z + matrix.pos.y,
            matrix.right.z * point.x + matrix.up.z * point.y + matrix.at.z * point.z + matrix.pos.z
        };
    }

    inline double Cross(const RwV2d& a, const RwV2d& b, const RwV2d& c)
    {
        return (double(b.x) - a.x) * (double(c.y) - a.y)
             - (double(b.y) - a.y) * (double(c.x) - a.x);
    }

    struct Polygon
    {
        std::array<RwV2d, 5> points{};
        int count = 0;
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
        const auto end = std::unique(points.begin(), points.end(), [](const auto& a, const auto& b)
        {
            return a.x == b.x && a.y == b.y;
        });
        const int size = static_cast<int>(end - points.begin());
        if (size < 3)
            return {};

        std::array<RwV2d, 10> hull{};
        int count = 0;
        for (int i = 0; i < size; ++i)
        {
            while (count >= 2 && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        const int lowerCount = count;
        for (int i = size - 2; i >= 0; --i)
        {
            while (count > lowerCount && Cross(hull[count - 2], hull[count - 1], points[i]) <= 0.0)
                --count;
            hull[count++] = points[i];
        }
        --count; // the first point is repeated at the end
        if (count < 3)
            return {};

        Polygon result;
        result.count = count;
        std::copy_n(hull.begin(), count, result.points.begin());
        return result;
    }

    struct Footprint
    {
        std::array<RwV2d, 9> anchors{};
        std::array<std::array<RwV2d, 5>, 3> ranges{}; // far, LOD, priority
        bool valid = false;

        void Capture(const std::array<RwV3d, 9>& local, const RwV3d* world, const RwMatrix& matrix)
        {
            valid = false;
            if (!std::isfinite(local[1].z) || local[1].z <= 0.0f || local[5].z <= 0.0f)
                return;

            const float lodScale = local[5].z / local[1].z;
            const float priorityX = local[5].x != 0.0f ? local[7].x / local[5].x : 0.2f;
            const float priorityY = local[5].y != 0.0f ? local[7].y / local[5].y : 0.2f;
            for (size_t i = 0; i < anchors.size(); ++i)
                anchors[i] = ToSector(world[i]);
            for (auto& range : ranges)
                range[0] = anchors[0];

            for (size_t i = 1; i <= 4; ++i)
            {
                ranges[0][i] = anchors[i];
                auto corner = local[i] * lodScale;
                ranges[1][i] = ToSector(Transform(corner, matrix));
                corner.x *= priorityX;
                corner.y *= priorityY;
                ranges[2][i] = ToSector(Transform(corner, matrix));
            }
            // Use the exact native values for the pair that was already
            // transformed; avoid rounding differences at sector boundaries.
            const int left = local[5].y >= 0.0f ? 1 : 4;
            const int right = local[5].y >= 0.0f ? 2 : 3;
            ranges[1][left] = anchors[5];
            ranges[1][right] = anchors[6];
            ranges[2][left] = anchors[7];
            ranges[2][right] = anchors[8];
            valid = true;
        }

        Polygon Expand(const RwV2d* triangle, int count) const
        {
            if (!valid || count != 3)
                return {};
            const auto matches = [](const RwV2d& a, const RwV2d& b)
            {
                return std::abs(a.x - b.x) <= 0.001f && std::abs(a.y - b.y) <= 0.001f;
            };
            if (!matches(triangle[0], anchors[0]))
                return {};

            constexpr int pairs[][2] = { {1, 2}, {5, 6}, {7, 8} };
            for (size_t range = 0; range < ranges.size(); ++range)
            {
                if (!matches(triangle[1], anchors[pairs[range][0]])
                    || !matches(triangle[2], anchors[pairs[range][1]]))
                    continue;

                // ScanSectorPoly expects counterclockwise vertices. The old
                // triangle can even reverse its winding at steep pitches;
                // the hull supplies a consistent order for both scan edges.
                return ConvexHull(ranges[range]);
            }
            return {}; // leave modified or unrelated polygons alone
        }
    };
}
