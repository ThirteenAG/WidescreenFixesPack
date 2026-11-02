#pragma once
#include <algorithm>
#include <cmath>
#include <array>

namespace GodfatherCinematics
{
    struct Rectangle
    {
        int left = 0, top = 0, right = 0, bottom = 0;
    };

    inline float Animation(bool active, bool leaving, float elapsed, float duration, bool animated)
    {
        if (!active)
            return 0.0f;
        if (!animated || !std::isfinite(duration) || duration <= 0.0f || !std::isfinite(elapsed))
            return leaving ? 0.0f : 1.0f;
        const float progress = std::clamp(elapsed / duration, 0.0f, 1.0f);
        const float value = leaving ? 1.0f - progress : progress;
        return value * 0.125f > 0.001f ? value : 0.0f;
    }

    inline float ProjectionAspect(float aspect, int mode, float progress)
    {
        if (!std::isfinite(aspect) || aspect <= 0.0f)
            return 4.0f / 3.0f;
        if (!(mode & 2) || !std::isfinite(progress))
            return aspect;
        return aspect + (std::max(aspect, 4.0f / 3.0f) - aspect) * std::clamp(progress, 0.0f, 1.0f);
    }

    inline std::array<Rectangle, 4> Borders(int width, int height, int mode, float progress)
    {
        if (width <= 0 || height <= 0 || !std::isfinite(progress) || mode < 0 || mode > 3)
            return {};
        progress = std::clamp(progress, 0.0f, 1.0f);
        const double frameWidth = std::min(double(width), height * (4.0 / 3.0));
        const double frameHeight = std::min(double(height), width / (4.0 / 3.0));
        const double side = mode & 2 ? (width - frameWidth) * 0.5 * progress : 0.0;
        const double outerTop = mode & 2 ? (height - frameHeight) * 0.5 * progress : 0.0;
        const double authoredTop = mode & 1 ? ((mode & 2) ? frameHeight : height) * 0.125 * progress : 0.0;
        const int x = std::clamp(int(std::lround(side)), 0, width / 2);
        const int y = std::clamp(int(std::lround(outerTop + authoredTop)), 0, height / 2);
        return { Rectangle{ 0, 0, width, y }, Rectangle{ 0, height - y, width, height },
            Rectangle{ 0, y, x, height - y }, Rectangle{ width - x, y, width, height - y } };
    }
}
