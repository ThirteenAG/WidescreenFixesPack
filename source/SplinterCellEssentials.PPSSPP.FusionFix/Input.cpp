#include "Game.hpp"
namespace essentials {
namespace {
SafetyMipsInline directAxis, groundSpeed;
struct AxisCache {
    uintptr_t actor = 0;
    float *strafe = nullptr, *turn = nullptr, *forward = nullptr, *look = nullptr;
} axes;
struct ControllerLayout { unsigned info, weapon, armed, pawn, speed; } layout;
float Axis(uint8_t value) {
    const int raw = int(value) - 128;
    const float normalized = float(raw < -127 ? -127 : raw) / 127;
    const float magnitude = std::fabs(normalized);
    if (magnitude <= deadzone) return 0;
    const float response = (magnitude - deadzone) / (1 - deadzone);
    return normalized < 0 ? -response : response;
}
bool Sample(SceCtrlData& pad) { pad = {}; return sceCtrlPeekBufferPositive(&pad, 1) > 0; }
float MovementAmount() {
    SceCtrlData pad;
    if (!Sample(pad)) return 0;
    const float x = std::fabs(Axis(pad.Lx)), y = std::fabs(Axis(pad.Ly));
    return x > y ? x : y;
}
int DirectAxis(uintptr_t input, int axis, float value) {
    if (axis != 0xE0 && axis != 0xE1) return directAxis.call<int>(input, axis, value);
    const auto target = *reinterpret_cast<const uintptr_t*>(input + 0x12A0);
    if (!target) { axes = {}; return directAxis.call<int>(input, axis, value); }
    const auto actor = *reinterpret_cast<const uintptr_t*>(target + 0x34);
    if (!actor) { axes = {}; return directAxis.call<int>(input, axis, value); }
    if (axes.actor != actor || !axes.strafe || !axes.turn || !axes.forward || !axes.look) {
        // Cache native reflected property addresses, not copies of game objects.
        const auto method = *reinterpret_cast<const uintptr_t*>(input) + 0x130;
        const auto adjusted = input + *reinterpret_cast<const int16_t*>(method);
        const auto find = *reinterpret_cast<float* (**)(uintptr_t, uintptr_t, const char*)>(method + 4);
        if (!find) return directAxis.call<int>(input, axis, value);
        axes = {actor, find(adjusted, actor, "aStrafe"), find(adjusted, actor, "aTurn"),
                       find(adjusted, actor, "aForward"), find(adjusted, actor, "aLookUp")};
    }
    SceCtrlData pad;
    if (!Sample(pad) || (axis == 0xE0 ? !axes.strafe || !axes.turn : !axes.forward || !axes.look))
        return directAxis.call<int>(input, axis, value);
    if (axis == 0xE0) {
        if (axes.strafe) *axes.strafe = Axis(pad.Lx);
        if (axes.turn) *axes.turn = Axis(pad.Rsrv[0]);
    } else {
        if (axes.forward) *axes.forward = -Axis(pad.Ly);
        if (axes.look) *axes.look = Axis(pad.Rsrv[1]);
    }
    return 0;
}
void GroundSpeed(uintptr_t controller) {
    groundSpeed.call<void>(controller);
    const auto info = *reinterpret_cast<const uintptr_t*>(controller + layout.info);
    const auto weapon = *reinterpret_cast<const uintptr_t*>(controller + layout.weapon);
    if (info && weapon && *reinterpret_cast<const uint8_t*>(info + 0x2B31) == 3 &&
        (*reinterpret_cast<const uint32_t*>(weapon + layout.armed) & 1)) {
        const auto pawn = *reinterpret_cast<const uintptr_t*>(controller + layout.pawn);
        if (pawn) *reinterpret_cast<float*>(pawn + 0x3A0) =
            *reinterpret_cast<const float*>(controller + layout.speed) * MovementAmount();
    }
}
}
void InstallInput() {
    deadzone = console::bounded(inireader.ReadFloat("MAIN", "StickDeadzone", 0.1f), 0, 0.95f, 0.1f);
    {
        sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
        directAxis = safetymips::create_inline(sites::DirectAxis(), DirectAxis);
        // Hide the original left-stick camera help when using separate camera axes.
        injector::MakeJMP(pattern.get_first("B0 FF BD 27 B0 03 8C C4", 0), +[]() {});
    }
    if (inireader.ReadInteger("MAIN", "SpeedStickControl", 1)) {
        // The PSP controller tick snaps the stick magnitude to zero, walk or run
        // after the circular filter. Keep the filtered axes, so every native and
        // script speed calculation (holstered, armed, aiming) sees the real tilt.
        if (const auto walk = sites::WalkQuantization()) injector::MakeNOP(walk + 12, 8);
        const auto ground = sites::GroundSpeed();
        layout = sites::IsPsn(ground) ? ControllerLayout{0x9A0, 0x954, 0x5D0, 0x5D0, 0x978}
                                     : ControllerLayout{0x9AC, 0x964, 0x5E0, 0x5F0, 0x988};
        // The armed branch otherwise assigns a fixed speed and skips Bias.
        // Correct it after the native routine, including input below its threshold.
        groundSpeed = safetymips::create_inline(ground, GroundSpeed);
    }
}
}
