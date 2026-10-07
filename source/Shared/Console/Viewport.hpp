#pragma once
#include <cmath>

namespace console {
struct Point { float x, y; };
struct Rect { float left, bottom, right, top; };
static_assert(sizeof(Rect) == 16);

// Coordinates remain in the game's canvas; only conversion to the displayed
// aspect ratio is changed. The same affine transform covers sprites, font
// buffers, radar masks and clip windows, independent of draw order.
struct Transform {
    float scaleX = 1.0f, scaleY = 1.0f;
    float offsetX = 0.0f, offsetY = 0.0f;
    static Transform anchored(float baseAspect, float aspect, float size,
                              float anchorX, float anchorY) {
        const float x = baseAspect / aspect * size;
        return {x, size, anchorX * (1.0f - x), anchorY * (1.0f - size)};
    }
    float x(float value) const { return value * scaleX + offsetX; }
    float y(float value) const { return value * scaleY + offsetY; }
    Rect rect(Rect value) const {
        return {x(value.left), y(value.bottom), x(value.right), y(value.top)};
    }
};

inline float bounded(float value, float low, float high, float fallback) {
    if (!std::isfinite(value)) return fallback;
    return value < low ? low : value > high ? high : value;
}

// Preserve zoom and scripted FOV changes in tangent space. Scaling all input
// angles by a multiplier calculated at 70 degrees changes zoom behavior.
template<class Tangent, class ArcTangent>
inline float horizontal_fov(float degrees, float baseAspect, float aspect, Tangent tangent, ArcTangent arcTangent) {
    if (!(degrees > 0.0f && degrees < 179.0f) || !(baseAspect > 0.0f && aspect > 0.0f)) return degrees;
    constexpr float radians = 0.01745329251994329577f;
    return arcTangent(tangent(degrees * (radians * 0.5f)) * (aspect / baseAspect)) * (2.0f / radians);
}
inline float horizontal_fov(float degrees, float baseAspect, float aspect) {
    return horizontal_fov(degrees, baseAspect, aspect,
        [](float value) { return std::tan(value); }, [](float value) { return std::atan(value); });
}
}
