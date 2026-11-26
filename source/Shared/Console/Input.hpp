#pragma once
#include <atomic>
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

// Versioned mouse export. The host is the sole writer of cumulative counters.
struct MouseMotion
{
    uint32_t generation, sequence, x, y, wheel, buttons;
};
static_assert(sizeof(MouseMotion) == 24 && alignof(MouseMotion) == 4);

struct MouseTransitions
{
    MouseMotion motion;
    uint32_t presses[5];
};
static_assert(sizeof(MouseTransitions) == 44 && alignof(MouseTransitions) == 4);

inline bool ReadMouseMotion(const volatile MouseMotion& source, MouseMotion& snapshot,
                            const volatile uint32_t* presses, uint32_t (&pressed)[5])
{
    const uint32_t sequence = source.sequence;
    if (sequence & 1u) return false;
    std::atomic_thread_fence(std::memory_order_acquire);
    snapshot = {source.generation, sequence, source.x, source.y, source.wheel, source.buttons};
    if (presses)
        for (unsigned i = 0; i < 5; ++i)
            pressed[i] = presses[i];
    std::atomic_thread_fence(std::memory_order_acquire);
    return source.sequence == sequence && source.generation == snapshot.generation;
}

struct Input {
    uint8_t keys[256]{}, previousKeys[256]{};
    Mouse mouse{}, previousMouse{};
    MouseMotion lastMotion{};
    uint32_t lastPresses[5]{}, mousePresses{};

    static uint32_t buttonMask(const Mouse& sample)
    {
        return (sample.left ? 1u : 0) | (sample.right ? 2u : 0) | (sample.middle ? 4u : 0) |
            (sample.extra1 ? 8u : 0) | (sample.extra2 ? 16u : 0);
    }

    void setButtons(uint32_t buttons)
    {
        mouse.left = (buttons & 1u) != 0;
        mouse.right = (buttons & 2u) != 0;
        mouse.middle = (buttons & 4u) != 0;
        mouse.extra1 = (buttons & 8u) != 0;
        mouse.extra2 = (buttons & 16u) != 0;
    }

    void sampleKeys(const char* keyboard) {
        std::memcpy(previousKeys, keys, sizeof(keys));
        std::memcpy(keys, keyboard, sizeof(keys));
        // Raw keyboard input distinguishes left/right modifiers. Game bindings
        // use their generic Windows virtual keys, accepting either side.
        keys[16] = uint8_t(keys[16] || keys[160] || keys[161]);
        keys[17] = uint8_t(keys[17] || keys[162] || keys[163]);
        keys[18] = uint8_t(keys[18] || keys[164] || keys[165]);
        previousMouse = mouse;
        mousePresses = 0;
    }
    void sample(const char* keyboard, Mouse& source) {
        sampleKeys(keyboard);
        mouse = source;
        // Consume displacement once. All game queries use this immutable frame.
        source.x = source.y = source.wheel = 0.0f;
        source.wheelUp = source.wheelDown = 0;
        if (!std::isfinite(mouse.x)) mouse.x = 0;
        if (!std::isfinite(mouse.y)) mouse.y = 0;
        if (!std::isfinite(mouse.wheel)) mouse.wheel = 0;
    }
    void sample(const char* keyboard, Mouse& legacy, const volatile MouseMotion& motion)
    {
        if (!motion.generation)
        {
            sample(keyboard, legacy);
            return;
        }
        sampleMotion(keyboard, motion, nullptr);
    }

    void sample(const char* keyboard, Mouse& legacy, const volatile MouseMotion& motion,
                const volatile MouseTransitions& transitions)
    {
        if (!transitions.motion.generation)
        {
            sample(keyboard, legacy, motion);
            return;
        }
        sampleMotion(keyboard, transitions.motion, transitions.presses);
    }

    void sampleMotion(const char* keyboard, const volatile MouseMotion& motion, const volatile uint32_t* presses)
    {
        sampleKeys(keyboard);
        mouse.x = mouse.y = mouse.wheel = 0.0f;
        mouse.wheelUp = mouse.wheelDown = 0;
        setButtons(lastMotion.buttons);
        MouseMotion current{};
        uint32_t pressed[5]{};
        if (!ReadMouseMotion(motion, current, presses, pressed) || !current.generation) return;
        if (current.generation != lastMotion.generation)
        {
            lastMotion = {};
            std::memset(lastPresses, 0, sizeof(lastPresses));
        }
        mouse.x = static_cast<float>(static_cast<int32_t>(current.x - lastMotion.x));
        mouse.y = static_cast<float>(static_cast<int32_t>(current.y - lastMotion.y));
        mouse.wheel = static_cast<float>(static_cast<int32_t>(current.wheel - lastMotion.wheel));
        mouse.wheelUp = mouse.wheel > 0;
        mouse.wheelDown = mouse.wheel < 0;
        if (presses)
        {
            for (unsigned i = 0; i < 5; ++i)
            {
                if (pressed[i] != lastPresses[i]) mousePresses |= 1u << i;
                lastPresses[i] = pressed[i];
            }
        }
        // A tap that ended between frames is held for this one guest frame.
        setButtons(current.buttons | mousePresses);
        lastMotion = current;
    }
    bool held(unsigned key) const { return key < 256 && keys[key] != 0; }
    bool pressed(unsigned key) const { return held(key) && previousKeys[key] == 0; }
    bool released(unsigned key) const { return key < 256 && !keys[key] && previousKeys[key]; }
    int axis(unsigned negative, unsigned positive, int range = 128) const {
        return (int(held(positive)) - int(held(negative))) * range;
    }
    // A key or mouse button went down, the mouse moved or the wheel turned in
    // this frame. Unlike active(), a state that never changes doesn't count.
    bool touched() const {
        if (mouse.x || mouse.y || mouse.wheel) return true;
        if (mousePresses || (buttonMask(mouse) & ~buttonMask(previousMouse))) return true;
        for (unsigned key = 1; key < 256; ++key) if (keys[key] && !previousKeys[key]) return true;
        return false;
    }
    bool active() const {
        if (mouse.x || mouse.y || mouse.wheel || buttonMask(mouse)) return true;
        for (auto key : keys) if (key) return true;
        return false;
    }
};
}
