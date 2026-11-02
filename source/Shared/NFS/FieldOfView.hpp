#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <vector>

namespace NFSFieldOfView
{
    inline uint32_t ScaleDrivingAngle(uint32_t value, float factor, float maximum)
    {
        factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return value;
        const auto original = static_cast<float>(value & 0xFFFF);
        const auto angle = original * factor;
        return (value & 0xFFFF0000) | static_cast<uint32_t>(std::clamp(angle, 0.0f, std::max(maximum, original)));
    }

    // Widest camera angle (65536 = 360 degrees), up to maximum, that keeps the horizontal field of view within 150
    // degrees. The projection turns the angle into the horizontal half-angle
    // atan(tan(angle * angleScale(angle) / 2) * tanScale); unbounded, the widest settings on wide screens approach
    // 180 degrees, where the tangent overflows and the picture breaks up.
    template<class AngleScale>
    inline float HorizontalLimit(float maximum, float tanScale, AngleScale angleScale)
    {
        constexpr float Pi = 3.14159265f;
        constexpr float MaxHalfTangent = 3.7320508f; // tan(75 degrees)
        const float halfLimit = std::atan(MaxHalfTangent / std::max(tanScale, 0.01f));
        auto halfAngle = [&](float angle) { return angle * angleScale(angle) * Pi / 65536.0f; };
        if (halfAngle(maximum) <= halfLimit)
            return maximum;
        float low = 0.0f;
        float high = maximum;
        for (int i = 0; i < 20; ++i)
        {
            const float middle = (low + high) * 0.5f;
            (halfAngle(middle) <= halfLimit ? low : high) = middle;
        }
        return low;
    }

    inline float HorizontalLimit(float maximum, float tanScale, float angleScale = 1.0f)
    {
        return HorizontalLimit(maximum, tanScale, [angleScale](float) { return angleScale; });
    }

    template<class Pattern>
    inline bool InstallDrivingHooks(Pattern& pattern, ptrdiff_t offset, size_t expected, std::vector<SafetyHookMid>& hooks,
        void(*callback)(SafetyHookContext&))
    {
        if (!hooks.empty())
            return true;
        pattern.count(expected);
        hooks.reserve(expected);
        for (size_t index = 0; index < expected; ++index)
            hooks.push_back(safetyhook::create_mid(pattern.get(index).template get<void>(offset), callback));
        return true;
    }
}
