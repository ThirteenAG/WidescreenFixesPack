module;

#include <stdafx.h>

export module Echelon;

import ComVars;

export void InitEchelon()
{
    // set player speed to max on game start
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "8B 4D ? 8B 79 ? 33 DB 47 8B C7 89 79 ? 80 38 42 89 65 ? 89 5D ? 75 ? 8B 41 ? 53 50 FF 15 ? ? ? ? D9 86",
        "8B 4D ? FF 41 ? 8B 41 ? 80 38 ? 75 ? 6A ? FF 71 ? FF 15 ? ? ? ? F3 0F 10 86 ? ? ? ? 83 EC ? F3 0F 10 8E ? ? ? ? F3 0F 59 C0 F3 0F 59 C9 F3 0F 58 C1 0F 5A C0 F2 0F 11 04 24 FF 15 ? ? ? ? F3 0F 10 25");
    static auto defautSpeedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (std::find(EchelonGameInfoPtrs.begin(), EchelonGameInfoPtrs.end(), regs.esi) != EchelonGameInfoPtrs.end())
        {
            *(uintptr_t*)(regs.esi + 0x778) = 5;
        }
        EchelonGameInfoPtrs.clear();
    });

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 8A ? ? ? ? F3 0F 10 81 ? ? ? ? 0F 2E C1 9F F6 C4 ? 0F 8A ? ? ? ? F3 0F 10 89");
    if (!pattern.empty())
        injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jp -> jmp
    else
    {
        pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 05 ? ? ? ? D9 81 ? ? ? ? DA E9 DF E0 F6 C4 ? 7A");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp
    }

    // Aiming camera smoothing
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "0F 84 ? ? ? ? 8B 86 ? ? ? ? DD D8", "0F 84 ? ? ? ? 8B 87 ? ? ? ? 83 EC ? F3 0F 10 80");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0xE990, true); // jz -> jmp
}