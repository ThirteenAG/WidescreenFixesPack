#pragma once
#include "Game.hpp"
#include "../Shared/Console/Input.hpp"
#include "../Shared/Console/StoriesMouse.hpp"
#include "../Shared/Console/StoriesBindings.hpp"
extern "C" {
#include "../../external/injector/include/ps2/pcsx2f_api.h"
}

namespace lcs {
// CControllerState, identical to the VCS layout (48 bytes).
struct Controller {
    int16_t leftX, leftY, rightX, rightY;
    int16_t l1, l2, r1, r2, up, down, left, right;
    int16_t unused[4];
    int16_t start, select, square, triangle, cross, circle, l3, r3;
};
// LCS PS2 CPad (188 bytes, CPad::GetPad(n) = 0x408C60 + 188 * n).
struct Pad {
    int16_t unused;
    Controller current, previous;
    int16_t steeringBuffer[10];   // +98: drunk steering delay, written by GetSteeringLeftRight
    int32_t drunkIndex;           // +120
    int16_t phase, mode, shakeDuration, disabled; // +124 .. +130
    uint8_t blockEnterVehicle, shakeFrequency;    // +132, +133
    uint8_t hornHistory[8], hornIndex, frontendFrames; // +134, +142, +143
    uint8_t pad144;
    char cheatHistory[12];        // +145
    uint8_t pad157[3];
    uint32_t lastTouched;         // +160
    uint8_t pad164[24];
};
static_assert(sizeof(Controller) == 48 && sizeof(Pad) == 188);
static_assert(offsetof(Pad, current) == 2 && offsetof(Pad, previous) == 50);
static_assert(offsetof(Pad, steeringBuffer) == 98 && offsetof(Pad, drunkIndex) == 120);
static_assert(offsetof(Pad, mode) == 126 && offsetof(Pad, disabled) == 130);
static_assert(offsetof(Pad, blockEnterVehicle) == 132 && offsetof(Pad, hornHistory) == 134);
static_assert(offsetof(Pad, frontendFrames) == 143 && offsetof(Pad, lastTouched) == 160);

namespace address {
constexpr uintptr_t pads = 0x408C60;
constexpr uintptr_t menuManager = 0x634610, menuActive = 0x63474D;
constexpr uintptr_t timeInMilliseconds = 0x3D9B70, timeStep = 0x3D9B8C;
constexpr uintptr_t camera = 0x43BBF0;
}
namespace key {
constexpr unsigned back = 8, tab = 9, enter = 13, shift = 16, ctrl = 17, alt = 18;
constexpr unsigned escape = 27, space = 32, pageUp = 33, pageDown = 34;
constexpr unsigned left = 37, up = 38, right = 39, down = 40;
constexpr unsigned num0 = 96, num2 = 98, num4 = 100, num5 = 101, num6 = 102, num8 = 104, plus = 107;
constexpr unsigned pause = 192; // ` (VK_OEM_3)
}
inline console::Input input;
inline console::stories::MouseMotion mouseMotion;
inline Pad* playerPad = nullptr;
using Act = console::stories::Action;
inline console::stories::Bindings bindings;
inline console::stories::Capture capture;
constexpr uint8_t game = console::stories::lcsOnly;
inline bool MenuActive() { return *reinterpret_cast<const uint8_t*>(address::menuActive) != 0; }
inline bool MouseControlsActive() {
    return settings.pcControls && playerPad && !playerPad->disabled && !MenuActive();
}
void InstallControls();
void InstallMouseCamera();
void InstallButtonIcons();
void InstallMenu();
void LoadBindings();
bool SaveBindings();
// Per-frame menu bookkeeping, from the pad update.
void TickMenu();
}
