module;

#include <stdafx.h>

export module Misc;

import ComVars;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            // The game's own command line when it is started without one
            static std::string defaultCmd("/B /lang:01  /spg:50 /GDBShaders KKMaps.bf");
            if (Screen.bSignatureEdition)
                defaultCmd = "/B /TX /lang:01 /spg:50 KingKongTheGame.bf";

            auto pattern = find_pattern("C7 45 ? ? ? ? ? 68 04 01 00 00 8D 85 D8 FB FF FF 50", "C7 45 FC 00 00 00 00 E8 ? ? ? ? 8B 45 10"); //0x401A10
            struct StartupHook
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint32_t*)(regs.ebp - 0x4) = 0;
                    std::string_view lpCmdLine(*(char**)(regs.ebp + 0x10));
                    if (lpCmdLine.empty())
                        *(uint32_t*)(regs.ebp + 0x10) = (uint32_t)defaultCmd.data();
                }
            }; injector::MakeInline<StartupHook>(pattern.get_first(0), pattern.get_first(7));

            // The Signature Edition opens a debugger window.
            if (Screen.bSignatureEdition)
            {
                pattern = hook::pattern("74 ? FF D0 85 C0");
                injector::WriteMemory<uint8_t>(pattern.get_first<uint8_t>(), 0xEB, true);
            }
        };
    }
} Misc;
