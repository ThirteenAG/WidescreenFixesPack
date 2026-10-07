#pragma once
#include <cmath>

// Mouse camera helpers shared by the GTA Stories PS2 PC control schemes.
namespace console::stories {
constexpr float pi = 3.14159265358979323846f, turn = 2.0f * pi;

// Raw-input pixels to a camera angle (radians). Positive horizontal output
// turns left (the game's beta convention); positive vertical output looks up.
// Mouse deltas are accumulated over the frame, so neither the timestep nor
// the pad's quadratic stick response belongs here.
inline float MouseAngle(float pixels, bool vertical, float sensitivity, bool invertY) {
    if (!std::isfinite(pixels)) return 0.0f;
    float angle = -pixels * sensitivity * (vertical ? 4.0f : 2.5f);
    if (!std::isfinite(angle)) return 0.0f;
    if (vertical && invertY) angle = -angle;
    // Extreme horizontal movement wraps by whole turns instead of reaching the
    // engine's angle clamp (CGeneral::LimitRadianAngle stops at +-25 radians).
    if (!vertical && (angle < -pi || angle > pi)) {
        angle = std::fmod(angle, turn);
        if (angle > pi) angle -= turn; else if (angle < -pi) angle += turn;
    }
    // A vertical step larger than a half turn is meaningless; the cameras clamp
    // alpha afterwards anyway.
    if (vertical) angle = angle > pi ? pi : angle < -pi ? -pi : angle;
    return angle;
}

// One frame of mouse motion. Each axis is applied once, by its first user,
// because the player and several cameras may query it in the same frame.
struct MouseMotion {
    float x = 0.0f, y = 0.0f;
    unsigned frame = 0;
    void sample(float horizontal, float vertical) { x = horizontal; y = vertical; ++frame; }
    bool moving() const { return x != 0.0f || y != 0.0f; }
    float take(bool vertical, float sensitivity, bool invertY) {
        float& pixels = vertical ? y : x;
        const float angle = MouseAngle(pixels, vertical, sensitivity, invertY);
        pixels = 0.0f;
        return angle;
    }
};

inline float WrapAngle(float angle) {
    if (!std::isfinite(angle)) return 0.0f;
    if (angle < -pi || angle > pi) {
        angle = std::fmod(angle, turn);
        if (angle > pi) angle -= turn; else if (angle < -pi) angle += turn;
    }
    return angle;
}
}
