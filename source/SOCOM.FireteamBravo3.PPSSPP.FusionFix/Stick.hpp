#pragma once

namespace Stick
{
inline console::Point Radial(float x, float y)
{
    if (!std::isfinite(x) || !std::isfinite(y))
        return {};
    x = std::clamp(x, -1.0f, 1.0f);
    y = std::clamp(y, -1.0f, 1.0f);
    constexpr float dead = 8689.0f / 32767.0f;
    const float magnitude = std::sqrt(x * x + y * y);
    if (magnitude <= dead)
        return {};
    const float scale = (std::min(magnitude, 1.0f) - dead) / (magnitude * (1.0f - dead));
    return { x * scale, y * scale };
}
}
