module;

#include <stdafx.h>
#include "FixSettings.hpp"

// Startup: IntroState shows the legal splash, then plays "Sting" (the studio logo) and "Intro" (the opening movie);
// PressStartStateV2 waits for Start. Skip intro leaves out the splash and the logo; the opening movie still plays and
// the game's own Enter/Esc skips it. Skip press start continues from the title as if Start was pressed, once.

export module Intro;


export namespace SRSIntro
{
    inline SafetyHookInline IntroHook, PressStartHook;
    inline bool SkipPressStart = false;

    inline int __fastcall Intro(uint8_t* state, void*)
    {
        const int result = IntroHook.thiscall<int>(state);
        if (*reinterpret_cast<void**>(state + 2108))
            *reinterpret_cast<float*>(state + 2120) = 5.0f; // the splash's fade timer at its end
        *reinterpret_cast<int32_t*>(state + 2092) = 1;      // the movie list continues after "Sting"
        return result;
    }

    inline int __fastcall PressStart(uint8_t* state, void*)
    {
        const int result = PressStartHook.thiscall<int>(state);
        if (std::exchange(SkipPressStart, false))
            state[3781] = 1; // continues without waiting for Start, as when the game is started for a network session
        return result;
    }

    inline void Init()
    {
        if (FixSettings().Bool("MAIN.SkipIntro"))
        {
            auto intro = hook::pattern("53 56 8B F1 8B 8E 38 08 00 00 57 8D BE 38 08 00 00 33 DB");
            IntroHook = safetyhook::create_inline(intro.get_first(), Intro);
        }
        SkipPressStart = FixSettings().Bool("MAIN.SkipPressStart");
        if (SkipPressStart)
        {
            auto pressStart = hook::pattern("53 56 57 8B F1 6A 01 6A 01 C6 86 38 0E 00 00 01 E8");
            PressStartHook = safetyhook::create_inline(pressStart.get_first(), PressStart);
        }
    }
}
