module;

#include <stdafx.h>
#include <cmath>

export module DrawDistance;

import ComVars;

export void InitDrawDistance()
{
    auto pDrawDistance = find_pattern("8B 8C 24 B4 00 00 00 8B 03 51 68 CD CC 4C 3D 53");
    auto pDrawDistValue = find_pattern("68 00 00 96 43 68 CD CC CC 3D 8B 51 10 8B 82 7C 01 00 00");
    auto pDrawDistValue1 = find_pattern("68 00 00 48 43 4F 52 50 57 8B CE 83 ED 74 E8 ? ? ? ?");
    auto pDrawDistValue2 = find_pattern("D8 05 ? ? ? ? D9 44 24 28 D8 4C 24 28 D9 44 24 24");
    auto pDrawDistValue22 = find_pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 7B ? D9 44 24 44");
    auto pDrawDistValue23 = find_pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 7B ? D9 44 24 3C");
    auto pDrawDistValue24 = find_pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 7A ? EB ? 33 ED");
    auto pDrawDistValue25 = find_pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 7B ? 83 C5 08");
    auto pDrawDistValue12 = find_pattern("C7 44 24 14 00 00 48 43 C7 44 24 50 00 00 66 43");
    auto pDrawDistValue26 = find_pattern("C7 44 24 50 00 00 66 43 8B 11 89 56 2C 8B 41 04");

    if (pDrawDistance.size() != 1 ||
        pDrawDistValue.size() != 1)
        return;
    if (nGameVersion == MAFIA_1_0_ENG && (pDrawDistValue1.size() != 1 || pDrawDistValue2.size() != 1 || pDrawDistValue22.size() != 1 || pDrawDistValue23.size() != 1 || pDrawDistValue24.size() != 1 || pDrawDistValue25.size() != 1)) return;
    if (nGameVersion != MAFIA_1_0_ENG && (pDrawDistValue12.size() != 1 || pDrawDistValue26.size() != 1)) return;

    static float drawDistValue1 = fDrawDistance - 50.0f;
    static float drawDistValue2 = fDrawDistance - 20.0f;

    static auto DrawDistanceHook = safetyhook::create_mid(pDrawDistance.get_first(), [](SafetyHookContext& regs)
    {
        auto& distance = Field<float>(regs.esp, 0xB4);
        if (distance > fDrawDistanceMin && distance < fDrawDistanceMax)
            distance = fDrawDistance;
    });

    if ((300.0f > fDrawDistanceMin) && (300.0f < fDrawDistanceMax))
    {
        injector::WriteMemory(pDrawDistValue.get_first(0x1), fDrawDistance, true);
    }

    if (nGameVersion == MAFIA_1_0_ENG)
    {
        injector::WriteMemory(pDrawDistValue1.get_first(0x1), drawDistValue1, true);

        injector::WriteMemory(pDrawDistValue2.get_first(0x2), &drawDistValue2, true);
        injector::WriteMemory(pDrawDistValue22.get_first(0x2), &drawDistValue2, true);
        injector::WriteMemory(pDrawDistValue23.get_first(0x2), &drawDistValue2, true);
        injector::WriteMemory(pDrawDistValue24.get_first(0x2), &drawDistValue2, true);
        injector::WriteMemory(pDrawDistValue25.get_first(0x2), &drawDistValue2, true);
    }
    else
    {
        injector::WriteMemory(pDrawDistValue12.get_first(0x4), drawDistValue1, true);

        injector::WriteMemory(pDrawDistValue26.get_first(0x4), drawDistValue2, true);
    }
}
