module;

#include <stdafx.h>
#include <d3d9.h>

export module Misc;

import ComVars;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            auto bSkipIntro = iniReader.ReadInteger("MAIN", "SkipIntro", 1) != 0;

            if (bSkipIntro)
            {
                static auto bLegalScreenShown = *hook::get_pattern<uint8_t*>("38 1D ? ? ? ? 0F 85 ? ? ? ? C7 45", 2);
                auto pattern = hook::pattern("0F 84 ? ? ? ? 48 0F 84 ? ? ? ? 48 48 74 13");

                struct SkipIntroMovie
                {
                    void operator()(injector::reg_pack& regs)
                    {
                        *bLegalScreenShown = 1;
                    }
                }; injector::MakeInline<SkipIntroMovie>(pattern.get_first(0), pattern.get_first(6));
            }
        };
    }
} Misc;