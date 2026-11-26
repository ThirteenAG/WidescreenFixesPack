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
            auto& settings = FixSettings();
            const auto updateCursor = []()
            {
                static std::mutex mutex;
                std::lock_guard lock(mutex);
                CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
            };
            settings.OnChange("MAIN.ConfineCursor", updateCursor);
            updateCursor();
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
