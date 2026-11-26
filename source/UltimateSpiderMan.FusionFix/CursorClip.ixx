module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module CursorClip;

import ComVars;

namespace
{
    std::atomic_bool RefreshConfinement{ true };
    bool ConfineCursor = true;

    void ApplyConfinement()
    {
        if (RefreshConfinement.exchange(false, std::memory_order_relaxed))
            ConfineCursor = ReadNumericSetting("MAIN.ConfineCursor", 1) != 0;
        CursorConfinement::SetEnabled(ConfineCursor && !USMOverlayBlocksInput());
        CursorConfinement::Frame(hWnd.is_initialized() ? hWnd.get() : nullptr);
    }

    BOOL WINAPI GameClipCursor(const RECT* rect)
    {
        if (rect)
            ApplyConfinement();
        else
            CursorConfinement::Release();
        return TRUE;
    }
}

class CursorClip
{
public:
    CursorClip()
    {
        WFP::onInitEvent() += []()
        {
            FixSettings().OnChange("MAIN.ConfineCursor", []()
            {
                RefreshConfinement.store(true, std::memory_order_relaxed);
            });
            IATHook::Replace(GetModuleHandle(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", GameClipCursor));
            WFP::onGameProcessEvent() += ApplyConfinement;
        };
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
} CursorClip;
