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
    if (!bIsEnhanced)
    {
        UIntOverrides::Register(L"IntProperty Echelon.EchelonGameInfo.m_defautSpeed", +[]() -> int
        {
            return 5;
        });
    }

    // Camera acceleration
    pattern = find_module_pattern(GetModuleHandle(L"Echelon"), "7A ? D9 81 DC 03 00 00");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); // jp -> jmp
}
