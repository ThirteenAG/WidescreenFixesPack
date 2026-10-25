#pragma once
#include "../Shared/Console/PS2Settings.hpp"
#include "../Shared/Console/Viewport.hpp"
#include "../Shared/Console/Text.hpp"
#include <cstring>
#include <cstdlib>
#include <cmath>

namespace vcs {
namespace config {
inline console::ps2settings::Value values[] = {
    {"MAIN", "SkipIntro", "1"},
    {"MAIN", "ImprovedWidescreenSupport", "1"},
    {"MAIN", "Enable60FPS", "0"},
    {"MAIN", "UnthrottleEmuDuringLoading", "1"},
    {"CONTROLS", "ModernControlScheme", "1"},
    {"CONTROLS", "PCControlScheme", "0"},
    {"MAIN", "MouseSensitivity", "1.0"},
    {"MAIN", "InvertMouseY", "0"},
    {"MAIN", "HudScale", "1.0"},
    {"MAIN", "CutsceneBorders", "3"},
    {"MAIN", "TextLanguage", "auto"},
    {"MAIN", "FOVFactor", "1.0"},
    {"MAIN", "HudAspectRatioConstraint", "Auto"},
};
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
inline void Load() { (void)console::ps2settings::load(values); }
inline bool Boolean(unsigned index, bool fallback) {
    return console::ps2settings::boolean(values[index], fallback);
}
}
}
