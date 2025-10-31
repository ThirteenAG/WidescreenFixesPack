module;

#include <stdafx.h>

export module Echelon;

import ComVars;

export void InitEchelon()
{
    // Hud Helpers
    auto pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? 8B 89 A8 02 00 00");
    static auto AETextureManagerDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "85 C0 7C ? D9 44 24 08");
    static auto AETextureManagerexecDrawTileFromManagerHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        curDrawTileManagerTextureIndex = regs.eax;
    });

    // set player speed to max on game start
    pattern = hook::module_pattern(GetModuleHandle(L"Echelon"), "8B 48 ? 41 89 48 ? 80 39 42 75 ? 41 6A 00 89 48 ? 8B 48 ? 50 A1 ? ? ? ? FF 90 ? ? ? ? D9 86 ? ? ? ? D9 86 ? ? ? ? D9 C0 D8 C9 D9 C2 D8 CB 83 EC 08 DE C1 DD 1C 24 DD D8 DD D8 FF 15 ? ? ? ? D8 15");
    static auto defautSpeedHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        if (std::find(EchelonGameInfoPtrs.begin(), EchelonGameInfoPtrs.end(), regs.esi) != EchelonGameInfoPtrs.end())
        {
            *(uintptr_t*)(regs.esi + 0x5BC) = 5;
        }
        EchelonGameInfoPtrs.clear();
    });

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 81 DC 03 00 00");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp
}
