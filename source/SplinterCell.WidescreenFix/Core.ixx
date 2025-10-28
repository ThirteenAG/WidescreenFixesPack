module;

#include <stdafx.h>

export module Core;

import ComVars;

export void InitCore()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"Core"), "8B D9 C1 E9 02 83 E3 03 F3 A5 8B CB F3 A4 5B 5F 5E 59 8B 44 24 ? 8B 4C 24 ? 3B C1");
    static auto UObjectInitPropertiesHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        EchelonGameInfoPtrs.push_back(regs.ebp);
    });
}
