#pragma once
#include "Shared/Console/PS2Settings.hpp"
#include "Shared/Console/Viewport.hpp"
#include "Shared/Console/Text.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>

namespace lcs {
namespace config {
inline constexpr console::ps2settings::Value defaults[] = {
    {"MAIN", "SkipIntro", "1"},
    {"MAIN", "FPSLimit", "30"},
    {"MAIN", "UnthrottleEmuDuringLoading", "1"},
    {"MAIN", "DisableBlur", "1"},
    {"CONTROLS", "PCControlScheme", "0"},
    {"MAIN", "MouseSensitivity", "1.0"},
    {"MAIN", "InvertMouseY", "0"},
    {"MAIN", "HudScale", "1.0"},
    {"MAIN", "CutsceneBorders", "3"},
    {"MAIN", "TextLanguage", "auto"},
    {"MAIN", "FOVFactor", "1.0"},
    {"MAIN", "HudAspectRatioConstraint", "Auto"},
    {"MAIN", "ForceAspectRatio", "Auto"},
    {"MAIN", "GamepadIcons", "0"},
    {"MAIN", "VehicleLights", "1"},
    {"CONTROLS", "ModernControlScheme", "1"},
    {"MAIN", "GamepadCameraSpeed", "1.0"},
    {"MAIN", "InvertGamepadY", "0"},
    {"CONTROLS", "HoldVehicleCamera", "0"},
};
inline console::ps2settings::Value values[sizeof(defaults) / sizeof(*defaults)];
inline float Number(unsigned index, float fallback, float minimum, float maximum) {
    char* end = nullptr;
    const char* start = values[index].value;
    const float value = std::strtof(start, &end);
    if (end == start || !std::isfinite(value)) return fallback;
    const bool separated = *end == ' ' || *end == '\t';
    while (*end == ' ' || *end == '\t' || *end == '\r') ++end;
    if (*end && !(separated && *end == ';')) return fallback;
    return console::bounded(value, minimum, maximum, fallback);
}
inline void Load()
{
    std::memcpy(values, defaults, sizeof(values));
    (void)console::ps2settings::load(values);
}
inline bool Read()
{
    console::ps2settings::Value loaded[sizeof(defaults) / sizeof(*defaults)];
    std::memcpy(loaded, defaults, sizeof(loaded));
    if (!console::ps2settings::read(loaded))
        return false;
    std::memcpy(values, loaded, sizeof(values));
    return true;
}
inline bool Boolean(unsigned index, bool fallback) {
    return console::ps2settings::boolean(values[index], fallback);
}
}
}
