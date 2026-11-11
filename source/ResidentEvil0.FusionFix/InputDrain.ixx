module;

#include <stdafx.h>
#include <array>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <cstring>

export module InputDrain;

import EngineText;

export namespace RE0Input
{
    inline SafetyHookInline keyboardHook, padHook, mouseHook;
    inline std::function<bool()> overlayBlocksInput;
    inline bool keyboardHeld = false;
    inline bool mouseHeld = false;
    inline std::array<bool, 4> padHeld{};
    inline bool ready = false;

    inline bool Blocked()
    {
        if (overlayBlocksInput && overlayBlocksInput()) return true;
        DWORD process = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &process);
        return process != GetCurrentProcessId();
    }

    template<size_t Length> void Clear(void* object, size_t offset)
    {
        std::memset(static_cast<uint8_t*>(object) + offset, 0, Length);
    }

    inline void FilterKeyboard(void* keyboard, bool blocked, bool& held)
    {
        if (blocked) held = true;
        if (!held) return;
        bool down = false;
        for (size_t i = 0; i < 8; ++i) down |= RE0Text::Read<uint32_t>(keyboard, 288 + i * 4) != 0;
        // The derived game keyboard copies this complete block to its own UI cache after the native update.
        // Include repeat timers, and wait for release before letting a held overlay key reach gameplay.
        Clear<0x8C0>(keyboard, 288);
        if (!blocked && !down) held = false;
    }

    inline void FilterPad(void* manager, bool blocked, std::array<bool, 4>& held)
    {
        for (size_t i = 0; i < held.size(); ++i)
        {
            auto device = static_cast<uint8_t*>(manager) + 60 + 716 * i;
            if (blocked) held[i] = true;
            if (!held[i]) continue;
            const auto deadzone = std::clamp<int>(RE0Text::Read<int16_t>(device, 22), 0, 32767);
            const bool connected = RE0Text::Read<uint8_t>(device, 4) != 0 && RE0Text::Read<uint8_t>(device, 7) != 0;
            bool down = RE0Text::Read<uint32_t>(device, 348) != 0;
            for (size_t axis : { 372, 376, 380, 384 })
                down |= std::abs(static_cast<int64_t>(RE0Text::Read<int32_t>(device, axis))) > deadzone;
            const auto trigger = RE0Text::Read<uint8_t>(device, 21);
            down |= RE0Text::Read<uint8_t>(device, 388) > trigger || RE0Text::Read<uint8_t>(device, 389) > trigger;
            // Match the game's inactive-input cache. Device identity, native polling and rumble state stay native.
            Clear<44>(device, 348);
            if (!blocked && (!connected || !down)) held[i] = false;
        }
    }

    inline int __fastcall Keyboard(void* keyboard, void*)
    {
        const auto result = keyboardHook.thiscall<int>(keyboard);
        FilterKeyboard(keyboard, Blocked(), keyboardHeld);
        return result;
    }

    inline void FilterMouse(void* mouse, bool blocked, bool& held)
    {
        if (blocked) held = true;
        if (!held) return;
        const bool down = RE0Text::Read<uint32_t>(mouse, 56) != 0;
        // Keep cached cursor coordinates and device state; drain deltas, button edges and repeat timers.
        Clear<100>(mouse, 44);
        if (!blocked && !down) held = false;
    }

    inline int __fastcall Mouse(void* mouse, void*)
    {
        const auto result = mouseHook.thiscall<int>(mouse);
        FilterMouse(mouse, Blocked(), mouseHeld);
        return result;
    }

    inline int __fastcall Pad(void* manager, void*)
    {
        const auto result = padHook.thiscall<int>(manager);
        FilterPad(manager, Blocked(), padHeld);
        return result;
    }

    inline void Install()
    {
        auto keyboard = hook::pattern("83 EC 14 53 55 56 8D A9 20 01 00 00 57 89 4C 24 10 89 6C 24 20 8B D5 BE 08 00 00 00");
        auto pad = hook::pattern("53 55 8B E9 56 80 BD 75 0B 00 00 00 57 74 17 C6 85 75 0B 00 00 00 8B 0D");
        auto mouse = hook::pattern("83 EC 20 53 55 8B E9 56 8B 45 38 57 89 45 3C C7 45 38 00 00 00 00 C7 45 2C 00 00 00 00");
        keyboardHook = safetyhook::create_inline(keyboard.get_first(), Keyboard);
        padHook = safetyhook::create_inline(pad.get_first(), Pad);
        mouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
        ready = true;
    }
}
