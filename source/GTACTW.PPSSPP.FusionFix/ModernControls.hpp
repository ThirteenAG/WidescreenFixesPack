#pragma once
#include <cstdint>

namespace ctw {
// CTW's internal button bits differ from sceCtrl's bit layout. These values
// are the physical button masks installed by the native controller setup.
namespace button {
inline constexpr uint16_t circle = 1, cross = 2, right = 256, left = 512, square = 2048;
}
inline uint16_t ModernMask(uint16_t mask, bool vehicle, uint16_t nativeFire, uint16_t nativeAim) {
    using namespace button;
    if (vehicle) {
        const uint16_t moved = cross | right | square | left;
        return uint16_t((mask & ~moved) | ((mask & cross) ? right : 0) |
            ((mask & right) ? cross : 0) | ((mask & square) ? left : 0) |
            ((mask & left) ? square : 0));
    }
    // Both native setup choices use Circle to fire, but they exchange L/R
    // aiming. Preserve all aliases and every constituent of compound masks.
    if (nativeFire != circle || (nativeAim != left && nativeAim != right)) return mask;
    if (nativeAim == left) {
        return uint16_t((mask & ~(circle | right)) |
            ((mask & circle) ? right : 0) | ((mask & right) ? circle : 0));
    }
    return uint16_t((mask & ~(circle | right | left)) |
        ((mask & circle) ? right : 0) | ((mask & right) ? left : 0) |
        ((mask & left) ? circle : 0));
}
}
