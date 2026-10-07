#pragma once
#include "../Shared/Console/PSP.hpp"
#include "Sites.hpp"
namespace essentials {
using namespace console::portable;
inline float aspect = 480.0f / 272.0f, hudScale = 1, fovFactor = 1, deadzone = 0.1f;
inline console::Transform hud;
inline void UpdateDisplay() {
    aspect = Aspect();
    hud = console::Transform::anchored(480.0f / 272.0f, aspect, hudScale, 240, 136);
}
inline void UnprojectHud(float* point) {
    point[0] = (point[0] - hud.offsetX) / hud.scaleX;
    point[1] = (point[1] - hud.offsetY) / hud.scaleY;
}
void InstallInput();
void InstallHud();
void InstallCamera();
void InstallTiming();
void InstallLoaderScreens();
void InstallLoadingScreens();
void InstallLoaderRestart(uintptr_t text, size_t size);
}
