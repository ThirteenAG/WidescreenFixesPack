#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cmath>

namespace console {
// This layout is shared with PCSX2PluginInjector's existing raw-input export.
struct Mouse {
    int8_t left, right, middle, wheelUp, wheelDown, extra1, extra2;
    float wheel, x, y;
};
static_assert(sizeof(Mouse) == 20 && offsetof(Mouse, x) == 12);

struct Input {
    uint8_t keys[256]{}, previousKeys[256]{};
    Mouse mouse{}, previousMouse{};
    void sample(const char* keyboard, Mouse& source) {
        std::memcpy(previousKeys, keys, sizeof(keys));
        std::memcpy(keys, keyboard, sizeof(keys));
        // Raw keyboard input distinguishes left/right modifiers. Game bindings
        // use their generic Windows virtual keys, accepting either side.
        keys[16] = uint8_t(keys[16] || keys[160] || keys[161]);
        keys[17] = uint8_t(keys[17] || keys[162] || keys[163]);
        keys[18] = uint8_t(keys[18] || keys[164] || keys[165]);
        previousMouse = mouse;
        mouse = source;
        // Consume displacement once. All game queries use this immutable frame.
        source.x = source.y = source.wheel = 0.0f;
        source.wheelUp = source.wheelDown = 0;
        if (!std::isfinite(mouse.x)) mouse.x = 0;
        if (!std::isfinite(mouse.y)) mouse.y = 0;
        if (!std::isfinite(mouse.wheel)) mouse.wheel = 0;
    }
    bool held(unsigned key) const { return key < 256 && keys[key] != 0; }
    bool pressed(unsigned key) const { return held(key) && previousKeys[key] == 0; }
    bool released(unsigned key) const { return key < 256 && !keys[key] && previousKeys[key]; }
    int axis(unsigned negative, unsigned positive, int range = 128) const {
        return (int(held(positive)) - int(held(negative))) * range;
    }
    bool active() const {
        if (mouse.x || mouse.y || mouse.wheel || mouse.left || mouse.right || mouse.middle) return true;
        for (auto key : keys) if (key) return true;
        return false;
    }
};
}
