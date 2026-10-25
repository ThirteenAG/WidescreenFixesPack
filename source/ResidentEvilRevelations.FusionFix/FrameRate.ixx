module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module FrameRate;

import ComVars;

// Frame rate. The game's Frame Rate option offers VARIABLE (capped at 120 FPS), 60 and 30. With VARIABLE the fix caps
// the game at MAIN.FPSLimit instead: 0 = unlimited (1000), -1 = the monitor's refresh rate, or a number.

namespace
{
    ProtectedGameRef<int32_t> nVariableCap;  // the game's table of the option's caps: VARIABLE, 60, 30
    GameRef<uintptr_t> pMain;               // the game's main object: the frame rate cap in effect at +0x3C
    int32_t nCap = 120;

    int32_t GetFrameRateCap()
    {
        int32_t limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == 0)
            return 1000;
        if (limit < 0)
        {
            DEVMODEW mode = { .dmSize = sizeof(DEVMODEW) };
            limit = EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ? (int32_t)mode.dmDisplayFrequency : 60;
        }
        return std::clamp(limit, 30, 1000);
    }

    void Apply()
    {
        const auto old = nCap;
        nCap = GetFrameRateCap();
        if (nVariableCap.is_initialized())
            nVariableCap = nCap;

        // VARIABLE is in effect: change the running cap too
        if (pMain.is_initialized())
        {
            if (auto main = pMain.get())
            {
                auto& cap = *(float*)(main + 0x3C);
                if (cap == (float)old)
                    cap = (float)nCap;
            }
        }
    }
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("DB 04 85 ? ? ? ? 5E");
            if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: FrameRate"))
                nVariableCap.SetAddress(*pattern.get_first<int32_t*>(3));

            pattern = hook::pattern("A1 ? ? ? ? 0F 5B C0 F3 0F 11 40 3C");
            if (PatternAvailable(pattern, "ResidentEvilRevelations.FusionFix: FrameRate"))
                pMain.SetAddress(*pattern.get_first<uintptr_t*>(1));

            Apply();
            FixSettings().OnChange("MAIN.FPSLimit", Apply);
        };
    }
} FrameRate;
