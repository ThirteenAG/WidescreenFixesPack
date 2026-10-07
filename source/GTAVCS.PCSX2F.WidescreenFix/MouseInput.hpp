#pragma once
#include <cmath>

namespace vcs {
// Match the previous PC camera's displacement units. Mouse deltas have already
// accumulated over the frame, so neither timestep nor pad smoothing belongs here.
inline float MouseAngle(float pixels, bool vertical, float sensitivity, bool invertY) {
    if (!std::isfinite(pixels)) return 0.0f;
    float angle = -pixels * sensitivity * (vertical ? 4.0f : 2.5f);
    if (!std::isfinite(angle)) return 0.0f;
    if (vertical && invertY) angle = -angle;
    // Preserve ordinary raw deltas exactly; extreme movement may wrap by full
    // turns, without clipping it to an artificial maximum speed.
    constexpr float pi = 3.14159265358979323846f, turn = 2.0f * pi;
    if (!vertical && (angle < -pi || angle > pi)) angle = std::fmod(angle, turn);
    return angle;
}

struct MouseMotion {
    float x = 0.0f, y = 0.0f;
    void sample(float horizontal, float vertical) { x = horizontal; y = vertical; }
    float take(bool vertical, float sensitivity, bool invertY) {
        float& pixels = vertical ? y : x;
        const float angle = MouseAngle(pixels, vertical, sensitivity, invertY);
        pixels = 0.0f;
        return angle;
    }
};
}
