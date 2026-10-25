module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/CursorConfinement.hpp"

export module CursorClip;

import ComVars;

class CursorClip
{
public:
    CursorClip()
    {
        WFP::onInitEvent() += []()
        {
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
