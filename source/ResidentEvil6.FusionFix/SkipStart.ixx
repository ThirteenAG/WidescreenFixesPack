module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <cmath>

export module SkipStart;

import InputDrain;

namespace TitlePrompt
{
    struct State
    {
        bool enabled = false, consumed = false, active = false, blocked = true;
        unsigned current = 0, requested = 0, step = 0;
        float elapsed = 0.0f;
        bool pending = false, visible = false;
    };

    inline bool Eligible(const State& state)
    {
        return state.enabled && !state.consumed && state.active && !state.blocked &&
            state.current == 4 && state.requested == 4 && state.step == 2 &&
            std::isfinite(state.elapsed) && state.elapsed >= 1.0f && !state.pending && state.visible;
    }
}

export namespace RE6Start
{
    inline std::atomic<bool> enabled{ true };
    inline bool consumed = false, ready = false;
    inline SafetyHookMid keyboardHook, mouseHook;

    inline bool Eligible(const void* object)
    {
        using RE6Input::Read;
        return TitlePrompt::Eligible({ true, consumed, (Read<uint32_t>(object, 16) & 7) == 2,
            !RE6Input::ready || RE6Input::Blocked() || RE6Input::keyboardBlocked.load(),
            Read<uint8_t>(object, 57), Read<uint8_t>(object, 547), Read<uint8_t>(object, 58), Read<float>(object, 592),
            Read<uint8_t>(object, 549) != 0, Read<uint8_t>(object, 550) != 0 && Read<uint8_t>(object, 551) != 0 });
    }

    inline void Install()
    {
        enabled.store(FixSettings().Bool("MAIN.SkipPressStart"));
        FixSettings().OnChange("MAIN.SkipPressStart", []() { enabled.store(FixSettings().Bool("MAIN.SkipPressStart")); });
        FixSettings().Availability("MAIN.SkipPressStart", [](std::string_view value) { return value == "0" || (ready && RE6Input::ready); });
        auto input = hook::pattern("8B 0D ? ? ? ? 8B 81 50 03 00 00 50 E8 ? ? ? ? 85 C0 75 ? 8B 0D ? ? ? ? E8 ? ? ? ? 84 C0 75 ? 6A FF 8B CE E8");
        keyboardHook = safetyhook::create_mid(input.get_first(18), [](SafetyHookContext& regs)
        {
            if (!Eligible(reinterpret_cast<const void*>(regs.esi))) return;
            if (regs.eax) consumed = true;
            else if (enabled.load())
            {
                regs.eax = 1;
                consumed = true;
            }
        });
        mouseHook = safetyhook::create_mid(input.get_first(33), [](SafetyHookContext& regs)
        {
            if ((regs.eax & 255) && Eligible(reinterpret_cast<const void*>(regs.esi))) consumed = true;
        });
        ready = keyboardHook && mouseHook;
        if (!ready)
        {
            keyboardHook.reset();
            mouseHook.reset();
        }
    }
}
