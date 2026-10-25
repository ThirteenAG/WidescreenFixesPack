module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"

export module FrameRate;

import ComVars;

// Frame rate. The game's Frame Rate option offers 30, 60 and VARIABLE, which caps the game at 120 FPS. With VARIABLE
// the fix caps it at MAIN.FPSLimit instead: 0 = unlimited (1000), -1 = the monitor's refresh rate, or a number.

namespace
{
    float fFrameRateCap = 120.0f;   // what VARIABLE stands for; the game's code reads it instead of its 120
    GameRef<uintptr_t> pMain;       // the game's main object: the frame rate cap in effect at +0x44
    ProtectedGameRef<int32_t> nVariableTableValue;

    float GetFrameRateCap()
    {
        int32_t limit = FixSettings().Int("MAIN.FPSLimit");
        if (limit == 0)
            return 1000.0f;
        if (limit < 0)
        {
            DEVMODEW mode = { .dmSize = sizeof(DEVMODEW) };
            if (EnumDisplaySettingsW(nullptr, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1)
                limit = (int32_t)mode.dmDisplayFrequency;
            else
                limit = 60;
        }
        return (float)std::clamp(limit, 30, 1000);
    }

    void Apply()
    {
        const float old = fFrameRateCap;
        fFrameRateCap = GetFrameRateCap();
        if (nVariableTableValue.is_initialized())
            nVariableTableValue = (int32_t)fFrameRateCap;

        // VARIABLE is in effect: change the running cap too
        if (pMain.is_initialized())
        {
            if (auto main = pMain.get())
            {
                auto& cap = *(float*)(main + 0x44);
                if (cap == old)
                    cap = fFrameRateCap;
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
            struct Ref { const char* pattern; ptrdiff_t offset; };
            static const Ref Refs[] =
            {
                { "F3 0F 10 05 ? ? ? ? 51 F3 0F 11 04 24 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? A1", 4 },
                { "F3 0F 10 05 ? ? ? ? 51 F3 0F 11 04 24 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 8B 0D", 4 },
                { "D9 05 ? ? ? ? 5F 5E C2 0C 00", 2 },
                { "F3 0F 10 05 ? ? ? ? F3 0F 11 40 ? EB 28", 4 },
                { "F3 0F 5C 05 ? ? ? ? F3 0F 10 0D ? ? ? ? 0F 54 C1", 4 },
            };

            fFrameRateCap = GetFrameRateCap();
            for (auto& ref : Refs)
            {
                auto pattern = hook::pattern(ref.pattern);
                if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: FrameRate"))
                    WriteGameValue(pattern.get_first(ref.offset), &fFrameRateCap);
            }

            auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 40 ? EB 28");
            if (PatternAvailable(pattern, "ResidentEvil6.FusionFix: FrameRate"))
                pMain.SetAddress(*pattern.get_first<uintptr_t*>(-4));

            // The value of VARIABLE in the table the game reads its config with
            hook::pattern("6A 03 53 53 68").for_each_result([](hook::pattern_match match)
            {
                auto entry = *match.get<uintptr_t>(5);
                if (std::string_view((const char*)injector::ReadMemory<uint32_t>(entry, true)) == "VARIABLE")
                    nVariableTableValue.SetAddress(entry + 4);
            });

            Apply();
            FixSettings().OnChange("MAIN.FPSLimit", Apply);
        };
    }
} FrameRate;
