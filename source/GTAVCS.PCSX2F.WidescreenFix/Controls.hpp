#pragma once
#include "Game.hpp"
#include "../Shared/Console/Input.hpp"
#include "MouseInput.hpp"
#include "../Shared/Console/StoriesBindings.hpp"
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
}

namespace vcs {
struct Controller {
    int16_t leftX, leftY, rightX, rightY;
    int16_t l1, l2, r1, r2, up, down, left, right;
    int16_t unused[4];
    int16_t start, select, square, triangle, cross, circle, l3, r3;
};
struct Pad {
    int16_t unused;
    Controller current, previous;
    uint8_t pad98[42];
    int32_t drunkBuffer;
    int16_t phase, mode, shakeDuration, disabled;
    uint8_t applyBrakes, shakeFrequency, hornHistory[8], hornIndex, frontendFrames, brakes2;
    char cheatHistory[12];
    uint8_t alignment[3];
    uint32_t lastTouched;
    uint8_t pad184[16];
    int32_t confirmTime;
    float sensitivityX, sensitivityY;
};
static_assert(sizeof(Controller) == 48 && sizeof(Pad) == 212);
static_assert(offsetof(Pad, current) == 2 && offsetof(Pad, previous) == 50);
static_assert(offsetof(Pad, disabled) == 150 && offsetof(Pad, lastTouched) == 180);
namespace key {
constexpr unsigned back = 8, tab = 9, enter = 13, shift = 16, ctrl = 17, alt = 18;
constexpr unsigned escape = 27, space = 32, left = 37, up = 38, right = 39, down = 40;
constexpr unsigned num0 = 96, num2 = 98, num4 = 100, num5 = 101, num6 = 102, num8 = 104, plus = 107;
constexpr unsigned pause = 192;
}
inline console::Input input;
inline MouseMotion mouseMotion;
inline Pad* playerPad = nullptr;
using Act = console::stories::Action;
inline console::stories::Bindings bindings;
inline console::stories::Capture capture;
constexpr uint8_t game = console::stories::vcsOnly;
inline bool MenuActive() {
    auto menu = reinterpret_cast<void* (*)()>(0x471400)();
    return reinterpret_cast<int (*)(void*)>(0x3B5130)(menu) != 0;
}
// The player is driving or riding (the vehicle prompts name vehicle actions).
inline bool PlayerInVehicle() {
    const auto ped = *reinterpret_cast<const uint32_t*>(0x4E4910);
    return ped && reinterpret_cast<int (*)(uintptr_t)>(0x219D50)(ped) != 0;
}
inline bool MouseControlsActive() {
    return settings.pcControls && playerPad && !playerPad->disabled && !MenuActive();
}
void InstallControls();
// Switches the PC control scheme at runtime (menu toggle).
void ApplyPcControls();
void LoadBindings();
bool SaveBindings();
void InstallMouseCamera();
void ApplyMouseCamera();
void UpdateCheats();
}
