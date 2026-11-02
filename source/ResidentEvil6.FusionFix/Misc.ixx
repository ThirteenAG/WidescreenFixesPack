module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import SkipStart;

// Startup logos, notices and the initial title prompt.

namespace
{
    // The game's alert request (system, alert, flag): the notice is confirmed at once instead of shown. The save data
    // loader waits for the alert result at +0x4EC94 to be 1 (OK).
    void __fastcall ConfirmNotice(uint8_t* system, void*, int32_t alert, char flag)
    {
        injector::WriteMemory(system + 0x4EC94, 1, true);
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();

            // Skip the Capcom and engine logos and the startup notice
            auto logos = hook::pattern("0F 84 ? ? ? ? 48 74 0C 8B 04 8D");
            auto logosEnd = hook::pattern("FF D0 89 87 ? ? ? ? 5F");
            auto notice = hook::pattern("0F 8C ? ? ? ? 83 FA 04");
            if (settings.Bool("MAIN.SkipIntro"))
            {
                injector::MakeNOP(logos.get_first(), 6);
                injector::MakeJMP(logos.get_first(), logosEnd.get_first(8));
                injector::WriteMemory<uint16_t>(notice.get_first(), 0xE990, true);
            }

            // The startup notices with only an OK button: "Load successful." after the save data loads, and the
            // auto-save notice
            if (settings.Bool("MAIN.SkipIntro"))
            {
                for (auto ref : { "8B 0D ? ? ? ? 6A 01 6A 08 E8 ? ? ? ? F3 0F 10 05",
                                  "8B 0D ? ? ? ? 6A 01 6A 0F E8 ? ? ? ? C6 86 85 04 00 00 09",
                                  "8B 0D ? ? ? ? 6A 01 6A 0F E8 ? ? ? ? C6 86 85 04 00 00 1B" })
                {
                    auto pattern = hook::pattern(ref);
                    injector::MakeCALL(pattern.get_first(10), ConfirmNotice, true);
                }
            }

            RE6Start::Install();
        };
    }
} Misc;
