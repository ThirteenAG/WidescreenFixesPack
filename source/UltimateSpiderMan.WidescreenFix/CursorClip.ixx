module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/CursorConfinement.hpp"

export module CursorClip;

import ComVars;

namespace
{
    BOOL WINAPI GameClipCursor(const RECT* rect)
    {
        if (rect)
            return ClipCursor(rect);
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
            IATHook::Replace(GetModuleHandle(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", GameClipCursor));
            auto& settings = FixSettings();
            CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
            settings.OnChange("MAIN.ConfineCursor", []()
            {
                CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            });
            WFP::onGameProcessEvent() += []()
            {
                CursorConfinement::Frame(hWnd.get());
            };
        };
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
} CursorClip;
