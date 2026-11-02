module;

#include <stdafx.h>
#include "FixSettings.hpp"

// The title screen plays the SEGA logo and the title's entrance (state 5), then waits for Start (state 6). At the
// first title after the game starts, MAIN.SkipIntro goes straight to the waiting title and MAIN.SkipPressStart takes
// the Start press as given, which opens the game data selection.

export module Intro;


export namespace SonicIntro
{
    inline SafetyHookInline StateHook;
    inline bool SkipIntro = false;
    inline bool SkipPressStart = false;
    inline bool IntroDone = false;
    inline bool PressDone = false;

    inline void __fastcall State(uintptr_t title, void*, int state)
    {
        StateHook.thiscall<void>(title, state);
        if (state == 5 && SkipIntro && !IntroDone)
        {
            IntroDone = true;
            *reinterpret_cast<int32_t*>(title + 44) = 6;
        }
        else if (state == 6)
        {
            IntroDone = true;
            if (SkipPressStart && !PressDone)
            {
                PressDone = true;
                *reinterpret_cast<int32_t*>(title + 128) = 1;
                *reinterpret_cast<int32_t*>(title + 132) = 1;
            }
            PressDone = true;
        }
    }

    inline void Init()
    {
        SkipIntro = FixSettings().Bool("MAIN.SkipIntro");
        SkipPressStart = FixSettings().Bool("MAIN.SkipPressStart");
        auto state = hook::pattern("8B 44 24 04 83 E8 05 74 14 48 75 1F C7 41 38 ? ? ? ? C7 41 34 00 00 00 00 C2 04 00");
        StateHook = safetyhook::create_inline(state.get_first(), &State);
    }
}
