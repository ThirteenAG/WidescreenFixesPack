module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"

export module CursorClip;

import ComVars;

class CursorClip
{
public:
    CursorClip()
    {
        WFP::onInitEvent() += []()
        {
            Driv3rSettings::Bind("MAIN.ConfineCursor", true, CursorConfinement::SetEnabled);
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
