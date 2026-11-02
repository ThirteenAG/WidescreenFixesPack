module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"


export module Focus;

import ComVars;

export namespace DPLFocus
{
    SafetyHookInline pumpHook;
    std::function<void()> onDeactivate;
    bool wasInactive = false;
    char __fastcall Pump(void* self, void*, uint8_t* quit, bool* inactive)
    {
        const auto result = pumpHook.unsafe_thiscall<char>(self, quit, inactive);
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
        auto pump = hook::pattern("55 8B EC 83 EC 3C 8A 41 20 84 C0 53 56 8B 75 08 57 C6 45 FF 00 88 06");
        pumpHook = safetyhook::create_inline(pump.get_first(), Pump);
        const bool available = static_cast<bool>(pumpHook);
        FixSettings().Availability("MAIN.PauseOnFocusLoss", [available](std::string_view) { return available; });
    }
}
