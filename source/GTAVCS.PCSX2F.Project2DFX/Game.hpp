#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/Lights.hpp"
#include "../Shared/Console/Viewport.hpp"

namespace vcsfx {
using Vector = console::Vector3;
struct Settings {
    bool lights = false, stars = true, traffic = true;
    unsigned limit = 900;
    float radius = 1.0f, range = 500.0f;
    float smallStars = 0.15f, mediumStars = 0.6f, largeStars = 1.2f, largeChance = 0.2f;
};
inline Settings settings;
template<class T> const T& read(uintptr_t address) { return *reinterpret_cast<const T*>(address); }
inline const Vector& cameraPosition() { return read<Vector>(0x6F44D0 + 0xA50); }
inline unsigned hour() { return read<uint8_t>(0x4CD148); }
inline unsigned minute() { return read<uint8_t>(0x4CD149); }
void InstallLights();
void InstallStars();
void InstallTraffic();
void RenderTraffic();
}
