module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module SkipStart;

import InputDrain;

export namespace RESkipStart
{
    inline SafetyHookMid prompt;
    inline bool passed = false;

    inline void Install()
    {
        if (FixSettings().Bool("MAIN.SkipPressStart"))
        {
            auto branch = hook::pattern("83 FE 01 0F 85 ? ? ? ? 6A 00 6A 08 C7 87 58 02 00 00 00 00 A0 40");
            prompt = safetyhook::create_mid(branch.get_first(3), [](SafetyHookContext& context)
            {
                if (passed || REInput::Blocked()) return;
                // Enter the native keyboard-confirm path without changing device or input caches.
                context.eflags |= 0x40;
                passed = true;
            });
        }
    }
}
