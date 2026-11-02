module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module FrameRate;

import ComVars;

// Frame rate. The game's Frame Rate option offers 30, 60 and VARIABLE, which caps the game at 120 FPS. With VARIABLE
// the fix caps it at MAIN.FPSLimit instead: 0 = unlimited (1000), -1 = the monitor's refresh rate, or a number.
// The game keeps its cap in main+0x44; the config property FrameRate reads and writes it as an integer, where 120 is
// VARIABLE. The fix keeps that value for the property, so the game's config still says VARIABLE.

namespace
{
    float fFrameRateCap = 120.0f;   // what VARIABLE stands for; the game's code reads it instead of its 120
    GameRef<uintptr_t> pMain;       // the game's main object: the frame rate cap in effect at +0x44
    std::atomic<bool> bVariable{ true };
    SafetyHookInline shGetFrameRate, shSetFrameRate;
    SafetyHookMid shMenuInit, shMenuApply;

    constexpr int32_t nVariable = 120;

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

    float& Cap()
    {
        return *(float*)(pMain.get() + 0x44);
    }

    void Apply()
    {
        const float old = fFrameRateCap;
        fFrameRateCap = GetFrameRateCap();

        // VARIABLE is in effect: change the running cap too, unless a scene has lowered it for now
        if (bVariable && pMain.is_initialized() && pMain.get() && Cap() == old)
            Cap() = fFrameRateCap;
    }

    // The FrameRate config property
    int32_t __stdcall GetFrameRate()
    {
        return bVariable ? nVariable : shGetFrameRate.stdcall<int32_t>();
    }

    void __stdcall SetFrameRate(int32_t value)
    {
        bVariable = value == nVariable;
        if (bVariable)
            Cap() = fFrameRateCap;
        else
            shSetFrameRate.stdcall<void>(value);
    }
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            fFrameRateCap = GetFrameRateCap();

            // The 120 the config reader returns for VARIABLE or a missing value, and the one the Graphics menu applies
            static const struct { const char* pattern; ptrdiff_t offset; } Refs[] =
            {
                { "F3 0F 10 05 ? ? ? ? 51 F3 0F 11 04 24 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? A1", 4 },
                { "F3 0F 10 05 ? ? ? ? 51 F3 0F 11 04 24 68 ? ? ? ? 68 ? ? ? ? B9 ? ? ? ? E8 ? ? ? ? 8B 0D", 4 },
                { "D9 05 ? ? ? ? 5F 5E C2 0C 00", 2 },
                { "F3 0F 10 05 ? ? ? ? F3 0F 11 40 ? EB 28", 4 },
            };
            for (auto& ref : Refs)
            {
                auto pattern = hook::pattern(ref.pattern);
                injector::WriteMemory(pattern.get_first(ref.offset), &fFrameRateCap, true);
            }

            auto pattern = hook::pattern("F3 0F 10 05 ? ? ? ? F3 0F 11 40 ? EB 28");
            pMain.SetAddress(*pattern.get_first<uintptr_t*>(-4));

            // The FrameRate property: the config stores VARIABLE as 120
            auto getter = hook::pattern("83 EC 0C A1 ? ? ? ? D9 7C 24 02 D9 40 44");
            auto setter = hook::pattern("8B 44 24 04 DB 44 24 04 85 C0 79 06 D8 05 ? ? ? ? 8B 0D ? ? ? ? D9 59 44 C2 04 00");
            shGetFrameRate = safetyhook::create_inline(getter.get_first(), GetFrameRate);
            shSetFrameRate = safetyhook::create_inline(setter.get_first(), SetFrameRate);

            // Graphics menu: shows VARIABLE while it is in effect, whatever the cap; remembers what is applied
            pattern = hook::pattern("C7 86 C4 01 00 00 04 00 00 00 A1 ? ? ? ? 8B 90 48 01 01 00 8B 82 54 01 00 00");
            shMenuInit = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                if (bVariable)
                    *(int32_t*)(regs.esi + 0x170) = 2;
            });
            pattern = hook::pattern("8B 86 70 01 00 00 3B 86 98 01 00 00 74 47 83 E8 00 74 2F 48 74 17 48 75 3C");
            shMenuApply = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                const auto choice = *(int32_t*)(regs.esi + 0x170);
                if (choice != *(int32_t*)(regs.esi + 0x198) && choice >= 0 && choice <= 2)
                    bVariable = choice == 2;
            });

            FixSettings().OnChange("MAIN.FPSLimit", Apply);
        };
    }
} FrameRate;
