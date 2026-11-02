module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>

export module HudLayout;

export namespace SCCHud
{
    struct Offsets
    {
        double left = 0.0, right = 0.0, top = 0.0, bottom = 0.0;
    };

    inline Offsets Calculate(double width, double height, double left, double top,
        double viewWidth, double viewHeight, double stageWidth, double stageHeight, double constraint)
    {
        if (width <= 0 || height <= 0 || viewWidth <= 0 || viewHeight <= 0 || stageWidth <= 0 || stageHeight <= 0)
            return {};
        const double safeWidth = constraint > 0 && std::isfinite(constraint) ?
            std::min(width, height * std::clamp(constraint, 4.0 / 3.0, 32.0 / 9.0)) : width;
        const double safeLeft = (width - safeWidth) * 0.5;
        return { (safeLeft - left) * stageWidth / viewWidth,
            (width - safeLeft - left - viewWidth) * stageWidth / viewWidth,
            -top * stageHeight / viewHeight,
            (height - top - viewHeight) * stageHeight / viewHeight };
    }
}
