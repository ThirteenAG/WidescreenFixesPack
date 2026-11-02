module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"


export module Focus;

import ComVars;

export namespace Driv3rFocus
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
        auto pump = hook::pattern("83 EC 3C 8A 41 1C 84 C0 55 56 8B 74 24 48 57 C6 44 24 0F 00 88 06");
        pumpHook = safetyhook::create_inline(pump.get_first(), Pump);
        const bool available = static_cast<bool>(pumpHook);
        FixSettings().Availability("MAIN.PauseOnFocusLoss", [available](std::string_view) { return available; });
    }
}
