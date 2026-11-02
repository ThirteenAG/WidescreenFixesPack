module;

#include <stdafx.h>
#include <algorithm>
#include <cmath>
#include <iterator>

export module CameraFrame;

export namespace CameraFrame
{
    constexpr float Reference = 16.0f / 9.0f;
    constexpr float Radians = 3.14159265358979323846f / 180.0f;

    inline bool Cinematic(int kind, int mode)
    {
        return kind == 8 || (kind == 2 && (mode == 248 || mode == 249 || mode == 250));
    }

    inline bool Gameplay(int kind, int mode)
    {
        constexpr int locomotion[] =
        {
            2, 5, 6, 10, 13, 19, 20, 23, 24, 27, 30, 31,
            32, 33, 36, 49, 57, 64, 68, 90, 92, 93, 94, 95,
            118, 122, 127, 132, 137, 142, 147, 152, 157, 162, 167, 172,
            177, 182, 187, 188, 194, 207, 208, 317, 320, 323, 329, 332
        };
        return kind == 2 && std::find(std::begin(locomotion), std::end(locomotion), mode) != std::end(locomotion);
    }

    inline bool Valid(float angle, float aspect)
    {
        return std::isfinite(angle) && angle > 0.0f && angle < 180.0f && std::isfinite(aspect) && aspect > 0.0f;
    }

    inline float Horizontal(float angle, float aspect)
    {
        return Valid(angle, aspect) ? 2.0f * std::atan(std::tan(angle * Radians * 0.5f) * aspect / Reference) / Radians : angle;
    }

    inline float Target(float angle, float factor, float aspect)
    {
        if (!Valid(angle, aspect))
            return angle;
        factor = std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        if (factor == 1.0f)
            return angle;
        const auto maximum = 2.0f * std::atan(std::tan(75.0f * Radians) * Reference / aspect) / Radians;
        return std::clamp(angle * factor, 1.0f, std::max(1.0f, maximum));
    }

    inline float SceneInput(float angle, float aspect, bool nativeCorrection, bool wideSupport, bool limit)
    {
        if (!Valid(angle, aspect))
            return angle;
        const bool nativeWide = nativeCorrection && aspect > Reference;
        if ((!wideSupport || nativeWide) && !limit)
            return angle;
        float horizontal = wideSupport || nativeWide ? Horizontal(angle, aspect) : angle;
        if (nativeWide && (!limit || horizontal <= 150.0f))
            return angle;
        if (limit)
            horizontal = std::min(horizontal, 150.0f);
        if (nativeWide)
            return 2.0f * std::atan(std::tan(horizontal * Radians * 0.5f) * Reference / aspect) / Radians;
        return horizontal;
    }
}
