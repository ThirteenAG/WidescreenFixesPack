#pragma once
#include "../CursorConfinement.hpp"

namespace NFSCursor
{
    inline void Initialize()
    {
        auto& settings = FixSettings();
        CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
        settings.OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
}
