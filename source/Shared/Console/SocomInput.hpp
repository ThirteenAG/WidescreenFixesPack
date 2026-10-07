#pragma once
#include "PSP.hpp"
namespace console::socom {
struct Input {
    SceCtrlData pad;
    uint32_t changed;
    uint8_t flags[4];
    float cameraX, cameraY, leftX, leftY, deadZone, sensitivity;
};
static_assert(offsetof(Input, deadZone) == 40);
inline float Axis(uint8_t raw, float deadZone, float sensitivity) {
    int value = int(raw) - 128;
    if (value < -127) value = -127;
    const int dead = int(bounded(deadZone, 0, 126, 0));
    if (value >= -dead && value <= dead) return 0;
    value += value < 0 ? dead : -dead;
    return float(value) * bounded(sensitivity, 0, 16, 0);
}
inline Point Right(const Input* input, bool camera) {
    if (!input) return {};
    Point result{Axis(input->pad.Rsrv[0], input->deadZone, input->sensitivity),
                 Axis(input->pad.Rsrv[1], input->deadZone, input->sensitivity)};
    if (camera) {
        const float norm = result.x * result.x + result.y * result.y;
        const float diagonal = norm > 0 ? 2 * result.x * result.y / norm : 0;
        const float cameraSensitivity = bounded(*reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(input) + 112), 0, 16, 1);
        const float response = cameraSensitivity / (1 + 0.28f * diagonal * diagonal);
        result.x *= response; result.y *= response;
    }
    return result;
}
inline float Camera(Input* input, int axis, char shaped) {
    const auto value = Right(input, shaped != 0);
    return axis == 0 ? value.x : -value.y;
}
inline void KeepAwake(const Input* input) {
    const auto value = Right(input, false);
    if (value.x != 0 || value.y != 0) scePowerTick(6);
}
}
