module;

#include <stdafx.h>
#include "FixSettings.hpp"


export module Focus;

import Window;
import Renderer;

export namespace NeoFocus
{
    inline SafetyHookInline TickHook, ClockHook;
    inline bool NativePaused = false, OwnClock = false;
    inline std::atomic<bool> Enabled{ true };

    inline double __stdcall Clock(int command)
    {
        if (command == 0 || command == 1 || command == 2)
        {
            NativePaused = command == 2;
            OwnClock = false;
        }
        return ClockHook.stdcall<double>(command);
    }

    inline int __cdecl Tick()
    {
        const auto window = NeoRenderer::Window.load();
        const bool pause = window && Enabled && (GetForegroundWindow() != window || IsIconic(window));
        if (pause)
        {
            if (!NativePaused && ClockHook)
            {
                ClockHook.stdcall<double>(2);
                NativePaused = true;
                OwnClock = true;
            }
            Sleep(10);
            return 0;
        }
        if (OwnClock)
        {
            ClockHook.stdcall<double>(1);
            NativePaused = false;
            OwnClock = false;
        }
        return TickHook.ccall<int>();
    }

    inline void Install()
    {
        Enabled = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            Enabled = FixSettings().Bool("MAIN.PauseOnFocusLoss");
        });
        auto tick = hook::pattern("A0 ? ? ? ? 84 C0 74 0F A1 ? ? ? ? C7 80 E0 02 00 00 00 00 00 00 A1");
        auto clock = hook::pattern("A1 ? ? ? ? 83 EC 08 53 55 33 ED 3B C5 56 57 75 38 8D 44 24 10");
        ClockHook = safetyhook::create_inline(clock.get_first(), Clock, SafetyHookInline::StartDisabled);
        (void)ClockHook.enable();
        if (ClockHook)
        {
            TickHook = safetyhook::create_inline(tick.get_first(), Tick, SafetyHookInline::StartDisabled);
            (void)TickHook.enable();
        }
    }
}
