module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Startup;

export namespace SaboteurStartup
{
    inline SafetyHookInline TransitionHook;

    inline int __fastcall Transition(uint32_t* sequence, void* edx)
    {
        // Boot states zero and one play the publisher logo and title trailer. Four is
        // their normal completion target; retain its loading screen and initialization.
        if (sequence[1] <= 1)
            sequence[1] = 4;
        return TransitionHook.unsafe_fastcall<int>(sequence, edx);
    }

    inline void Install()
    {
        if (!FixSettings().Bool("MAIN.SkipIntro"))
            return;
        auto pattern = hook::pattern("56 8B F1 8B 46 04 83 F8 05 0F 87 ? ? ? ? FF 24 85 ? ? ? ? C7 06 00 00 00 00 C7 46 04 FF FF FF FF");
        TransitionHook = safetyhook::create_inline(pattern.get_first(), Transition);
    }
}
