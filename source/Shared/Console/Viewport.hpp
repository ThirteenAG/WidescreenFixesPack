#pragma once
#include <cmath>
#include <cstdlib>

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
    // As above, but the canvas anchor lands on screenX (the edge of a
    // constrained HUD box) instead of staying at the canvas edge.
    static Transform anchored(float baseAspect, float aspect, float size,
                              float anchorX, float anchorY, float screenX) {
        const float x = baseAspect / aspect * size;
        return {x, size, screenX - anchorX * x, anchorY * (1.0f - size)};
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

// MAIN.HudAspectRatioConstraint: "Auto" (or anything unparsable) = 0, the HUD
// uses the whole screen; "W:H" or a decimal is clamped to 4:3 .. 32:9.
inline float parse_hud_aspect(const char* text) {
    if (!text) return 0.0f;
    char* end = nullptr;
    float value = std::strtof(text, &end);
    if (end == text || !(value > 0.0f)) return 0.0f;
    if (*end == ':') {
        const char* start = end + 1;
        const float denominator = std::strtof(start, &end);
        if (end == start || !(denominator > 0.0f)) return 0.0f;
        value /= denominator;
    }
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    if (*end && *end != ';') return 0.0f;
    return bounded(value, 4.0f / 3.0f, 32.0f / 9.0f, 0.0f);
}
// Where an edge-anchored HUD element goes when the HUD is kept in a centered
// box of the constraint aspect. Screens narrower than the box are unchanged.
inline float hud_edge(float anchorX, float width, float aspect, float constraint) {
    if (!(constraint > 0.0f) || !(aspect > constraint)) return anchorX;
    const float center = width * 0.5f;
    return center + (anchorX - center) * (constraint / aspect);
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
