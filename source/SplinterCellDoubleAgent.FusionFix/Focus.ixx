module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module Focus;

import ComVars;
import DeveloperOverlay;

namespace
{
    ptrdiff_t InactiveOffset = 0;
}

export namespace Focus
{
    void Update(void* engine)
    {
        CursorConfinement::Suspend(DeveloperOverlay::BlocksInput());
        CursorConfinement::Frame(hGameWindow);
        if (!InactiveOffset || !hGameWindow)
            return;
        const bool paused = IsIconic(hGameWindow) ||
            (GetForegroundWindow() != hGameWindow && FixSettings().Bool("MAIN.PauseOnFocusLoss"));
        // The native tick pauses the engine and sound while this flag is set.
        *reinterpret_cast<int*>(static_cast<uint8_t*>(engine) + InactiveOffset) = paused ? 1 : 0;
    }

    void Init()
    {
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        auto pattern = hook::module_pattern(GetModuleHandleW(L"Engine.dll"),
            "39 B3 ? ? ? ? 74 ? 39 35 ? ? ? ? 75 ? 39 73 4C 74 ? C7 05");
        InactiveOffset = *pattern.get_first<int32_t>(2);
    }
}
