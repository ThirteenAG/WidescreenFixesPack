#pragma once
#include <algorithm>
#include <cmath>

namespace burnout
{
// Rotate the native camera basis about its own position. The game rebuilds the
// basis every frame, so releasing the stick restores its own driving view.
inline bool RotateCameraBasis(float* matrix, float yaw, float pitch)
{
    if (!std::isfinite(yaw) || !std::isfinite(pitch)) return false;
    if (yaw == 0.0f && pitch == 0.0f) return false;
    const float cy = std::cos(yaw), sy = std::sin(yaw);
    const float cp = std::cos(pitch), sp = std::sin(pitch);
    float right[3], up[3], forward[3];
    for (int i = 0; i < 3; ++i)
    {
        right[i] = matrix[i];
        up[i] = matrix[4 + i];
        forward[i] = matrix[8 + i];
        if (!std::isfinite(right[i]) || !std::isfinite(up[i]) || !std::isfinite(forward[i])) return false;
    }
    for (int i = 0; i < 3; ++i)
    {
        const float rotatedForward = right[i] * sy + forward[i] * cy;
        matrix[i] = right[i] * cy - forward[i] * sy;
        matrix[4 + i] = up[i] * cp - rotatedForward * sp;
        matrix[8 + i] = up[i] * sp + rotatedForward * cp;
    }
    return true;
}
struct PadOrbit
{
    float yaw = 0, pitch = 0;
    static float Axis(float v)
    {
        if (!std::isfinite(v)) return 0;
        v = std::clamp(v, -1.0f, 1.0f);
        const float n = std::fabs(v);
        return n <= 0.12f ? 0.0f : std::copysign((n - 0.12f) / 0.88f, v);
    }
    void Update(float x, float y, float dt, float speed, bool invert)
    {
        if (!std::isfinite(dt)) dt = 0;
        dt = std::clamp(dt, 0.0f, 0.1f);
        x = Axis(x); y = Axis(y);
        if (invert) y = -y;
        if (x == 0 && y == 0)
        {
            const float fade = 1.0f / (1.0f + 6.0f * dt);
            yaw *= fade; pitch *= fade;
            if (std::fabs(yaw) < 0.0001f) yaw = 0;
            if (std::fabs(pitch) < 0.0001f) pitch = 0;
        }
        else
        {
            yaw = std::clamp(yaw + x * speed * 2.4f * dt, -2.0943951f, 2.0943951f);
            pitch = std::clamp(pitch + y * speed * 1.2f * dt, -0.6108652f, 0.6108652f);
        }
    }
};

inline bool ApplyCameraLook(float* matrix, float x, float y)
{
    if (!std::isfinite(x) || !std::isfinite(y)) return false;
    auto axis = [](float value)
    {
        value = std::clamp(value, -1.0f, 1.0f);
        const float magnitude = std::fabs(value);
        return magnitude <= 0.12f ? 0.0f : std::copysign((magnitude - 0.12f) / 0.88f, value);
    };
    return RotateCameraBasis(matrix, axis(x) * 2.0943951f, axis(y) * 0.6108652f);
}

}
