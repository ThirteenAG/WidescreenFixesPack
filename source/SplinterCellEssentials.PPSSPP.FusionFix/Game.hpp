#pragma once
#include "../Shared/Console/PSP.hpp"
#include "Sites.hpp"
#include <algorithm>
namespace essentials {
using namespace console::portable;
constexpr float nativeAspect = 480.0f / 272.0f;
// forced: MAIN.ForceAspectRatio (0 = the emulator's display); hudConstraint:
// MAIN.HudAspectRatioConstraint (0 = Auto, the whole screen).
inline float aspect = nativeAspect, hudScale = 1, fovFactor = 1, deadzone = 0.1f, forced = 0, hudConstraint = 0;
// The 480x272 frame is stretched to the display. screen keeps the native
// aspect in a centered box (menus, loading screens, projected markers);
// hudLeft..hudBottom are the HUD box edges in frame units.
inline console::Transform screen;
inline float hudLeft = 0, hudRight = 480, hudTop = 0, hudBottom = 272;
inline float scaleX = 1, scaleY = 1;
// Unreal FNames in this game are CRCs of the upper-cased name (FNameCRC).
constexpr uint32_t CrcEntry(uint32_t index) {
    uint32_t value = index << 24;
    for (int bit = 0; bit < 8; ++bit) value = (value & 0x80000000u) ? (value << 1) ^ 0x04C11DB7u : value << 1;
    return value;
}
constexpr uint32_t NameCrc(const char* name) {
    uint32_t value = 0;
    for (; *name; ++name) {
        uint32_t c = uint8_t(*name);
        if (c >= 'a' && c <= 'z') c -= 32;
        value = (value >> 8) ^ CrcEntry((value ^ c) & 0xFF);
    }
    return value;
}
inline void UpdateDisplay() {
    aspect = forced > 0 ? forced : Aspect();
    scaleX = std::min(1.0f, nativeAspect / aspect);
    scaleY = std::min(1.0f, aspect / nativeAspect);
    screen = {scaleX, scaleY, 240 * (1 - scaleX), 136 * (1 - scaleY)};
    // A box narrower than the native layout keeps the native layout.
    const float box = hudConstraint > 0 ? std::clamp(hudConstraint, nativeAspect, std::max(aspect, nativeAspect)) : aspect;
    const float half = 240 * std::min(1.0f, box / aspect);
    hudLeft = 240 - half; hudRight = 240 + half;
}
// Native frame position of a HUD element part, anchored to a side of the HUD
// box (-1 left/top, 0 centered, 1 right/bottom) and scaled by HudScale there.
inline float HudX(float x, int side) {
    const float scale = scaleX * hudScale;
    if (side < 0) return hudLeft + x * scale;
    if (side > 0) return hudRight - (480 - x) * scale;
    return 240 + (x - 240) * scale;
}
inline float HudY(float y, int side) {
    const float scale = scaleY * hudScale;
    if (side < 0) return hudTop + y * scale;
    if (side > 0) return hudBottom - (272 - y) * scale;
    return 136 + (y - 136) * scale;
}
inline void UnprojectScreen(float* point) {
    point[0] = (point[0] - screen.offsetX) / screen.scaleX;
    point[1] = (point[1] - screen.offsetY) / screen.scaleY;
}
void InstallInput();
bool InstallHud();
void InstallCamera();
void InstallTiming();
void InstallLoaderScreens();
void InstallLoadingScreens();
void InstallLoaderRestart(uintptr_t text, size_t size);
}
