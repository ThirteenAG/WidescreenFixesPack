#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/Lights.hpp"
#include "../Shared/Console/Viewport.hpp"

namespace lcsfx {
using Vector = console::Vector3;
struct Settings {
    bool lights = true, traffic = true;
    unsigned limit = 900;
    float radius = 1.0f, range = 500.0f;
};
inline Settings settings;
template<class T> const T& read(uintptr_t address) { return *reinterpret_cast<const T*>(address); }
inline const Vector& cameraPosition() { return read<Vector>(0x43BBF0 + 0x30); }
inline unsigned hour() { return read<uint8_t>(0x3D9B74); }
inline unsigned minute() { return read<uint8_t>(0x3D9B80); }
void InstallLights();
void InstallTraffic();
void RenderTraffic();

}
