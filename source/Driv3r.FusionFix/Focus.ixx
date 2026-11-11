module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"


export module Focus;

import ComVars;

export namespace Driv3rFocus
{
    SafetyHookInline pumpHook;
    SafetyHookMid stopWaitHook;
    std::function<void()> onDeactivate;
    bool wasInactive = false, quitting = false;
    char __fastcall Pump(void* self, void*, uint8_t* quit, bool* inactive)
    {
        const auto result = pumpHook.unsafe_thiscall<char>(self, quit, inactive);
        if (*quit) quitting = true;
        if (inactive)
        {
            if (*inactive && !wasInactive && onDeactivate) onDeactivate();
            wasInactive = *inactive;
        }
        if (inactive && !FixSettings().Bool("MAIN.PauseOnFocusLoss")) *inactive = false;
        return result;
    }
    void Install()
    {
        auto pump = hook::pattern("83 EC 3C 8A 41 1C 84 C0 55 56 8B 74 24 48 57 C6 44 24 0F 00 88 06");
        pumpHook = safetyhook::create_inline(pump.get_first(), Pump);

        // A stopped sound voice is released ten presented frames later. Once the pump reports quit
        // no frame is presented any more, so the wait would keep the shutdown spinning forever
        // whenever sounds are still playing (closing the window during gameplay).
        auto stopWait = hook::pattern("8B 8F 8C 01 00 00 83 C1 0A 3B 0D ? ? ? ? 73");
        static const auto release = reinterpret_cast<uintptr_t>(stopWait.get_first(17));
        stopWaitHook = safetyhook::create_mid(stopWait.get_first(), [](SafetyHookContext& regs)
        {
            if (quitting)
                regs.eip = release;
        });
    }
}
