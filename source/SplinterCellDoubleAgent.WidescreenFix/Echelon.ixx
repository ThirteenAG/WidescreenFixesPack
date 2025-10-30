module;

#include "stdafx.h"

export module Echelon;

import ComVars;

export void InitEchelon()
{
    CIniReader iniReader("");
    gBlacklistIndicators = iniReader.ReadInteger("BONUS", "BlacklistIndicators", 0);

    if (gBlacklistIndicators || bLightSyncRGB)
    {
        auto pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "8B 96 ? ? ? ? 8D 0C 80");
        struct BlacklistIndicatorsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.edx = *(uint32_t*)(regs.esi + 0x15B8);
                regs.ecx = regs.eax + regs.eax * 4;

                if (regs.ecx == 0)
                {
                    auto fPlayerLuminosity = *(float*)(regs.edx + regs.ecx * 4 + 4);
                    if (fPlayerLuminosity > 120.0f) fPlayerLuminosity = 120.0f;

                    auto v = 120.0f - fPlayerLuminosity;
                    if (v <= 10.0f) v = 10.0f;
                    gVisibility = ((float)v / 120.0f);
                    if (gBlacklistIndicators == 2)
                        gVisibility = 1.0f - ((float)v / 120.0f);
                }
            }
        }; injector::MakeInline<BlacklistIndicatorsHook>(pattern.get_first(0), pattern.get_first(6));


        pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "8B 81 ? ? ? ? 8B 54 24 04 89 02 8B 81 ? ? ? ? 8B 4C 24 08 89 01");
        struct GetPrimaryAmmoHook
        {
            void operator()(injector::reg_pack& regs)
            {
                curAmmoInClip = *(uint32_t*)(regs.ecx + 0x0B6C);
                curClipCapacity = *(uint32_t*)(regs.ecx + 0x0B6C + 4);
                regs.eax = curAmmoInClip;
            }
        }; injector::MakeInline<GetPrimaryAmmoHook>(pattern.get_first(0), pattern.get_first(6));
    }
}
