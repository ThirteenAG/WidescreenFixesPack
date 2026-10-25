module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module FrameRate;

import ComVars;

// Frame rate. The game's Frame Rate option offers 30, 60 and VARIABLE, which caps the game at 120 FPS. With VARIABLE
// the fix caps it at MAIN.FPSLimit instead: 0 = unlimited (1000), -1 = the monitor's refresh rate, or a number. The
// game builds its table of the three caps whenever it reads the option.

namespace
{
    std::vector<ProtectedGameRef<int32_t>> VariableCaps;
    GameRef<uintptr_t> pMain;   // the game's main object: the frame rate cap in effect at +0x3C
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
        for (auto& cap : VariableCaps)
            cap = nCap;

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
            // mov [esp+8], 120: the VARIABLE entry of the table
            for (auto ref : { "C7 44 24 08 78 00 00 00 83 F8 03 72 0B", "C7 44 24 08 78 00 00 00 83 F8 03 72 07" })
            {
                auto pattern = hook::pattern(ref);
                if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: FrameRate"))
                    VariableCaps.emplace_back(pattern.get_first<int32_t>(4));
            }

            auto pattern = hook::pattern("A1 ? ? ? ? 66 0F 5A C0 F3 0F 11 40 3C");
            if (PatternAvailable(pattern, "ResidentEvilRevelations2.FusionFix: FrameRate"))
                pMain.SetAddress(*pattern.get_first<uintptr_t*>(1));

            Apply();
            FixSettings().OnChange("MAIN.FPSLimit", Apply);
        };
    }
} FrameRate;
