module;

#include <stdafx.h>

import ComVars;

export module EchelonHUD;

import ComVars;

export void InitEchelonHUD()
{
    auto pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"), "D8 0D ? ? ? ? D9 5C 24 ? D9 85 ? ? ? ? D8 1D ? ? ? ? DF E0 25 00 01 00 00");
    injector::WriteMemory(pattern.get_first(2), &Screen.HUDScaleX, true);

    pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"), "D8 0D ? ? ? ? D9 5C 24 ? D9 85 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 41");
    injector::WriteMemory(pattern.get_first(2), &Screen.HUDScaleX, true);

    pattern = find_module_pattern(GetModuleHandle(L"EchelonHUD"), "D9 44 24 ? D8 8D ? ? ? ? D9 5C 24");
    static auto AEGameMenuHUDexecPostRender_s_MissionCompleteHook1 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        *(float*)(regs.esp + 0x30) = 1.0f;
    });
}
