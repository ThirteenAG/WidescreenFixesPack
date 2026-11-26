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
    std::atomic<bool> PauseOnFocusLoss{ true };
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
            (GetForegroundWindow() != hGameWindow && PauseOnFocusLoss.load());
        // The native tick pauses the engine and sound while this flag is set.
        *reinterpret_cast<int*>(static_cast<uint8_t*>(engine) + InactiveOffset) = paused ? 1 : 0;
    }

    void Init()
    {
        ObserveFixSetting("MAIN.PauseOnFocusLoss", asi_settings::detail::ParseBool, [](int value) { PauseOnFocusLoss.store(value != 0); });
        ObserveFixSetting("MAIN.ConfineCursor", asi_settings::detail::ParseBool, [](int value) { CursorConfinement::SetEnabled(value != 0); });
        auto pattern = hook::module_pattern(GetModuleHandleW(L"Engine.dll"),
            "39 B3 ? ? ? ? 74 ? 39 35 ? ? ? ? 75 ? 39 73 4C 74 ? C7 05");
        InactiveOffset = *pattern.get_first<int32_t>(2);
    }
}
