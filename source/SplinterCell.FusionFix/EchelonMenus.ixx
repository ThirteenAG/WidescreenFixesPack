module;

#include <stdafx.h>

export module EchelonMenus;

import ComVars;

export void InitEchelonMenus()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"EchelonMenus"), "48 83 F8 04 0F 87");
    static auto UEPCInGameDataDetailsMenuDrawRecon = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        ReconType = regs.eax;
    });

    pattern = find_module_pattern(GetModuleHandle(L"EchelonMenus"), "53 55 56 8B 74 24 ? 57 8B 1D");
    static auto UEPCMenuPageDrawPalmBackground = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        ReconType = -1;
    });
}
