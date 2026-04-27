#include "stdafx.h"
#include <GTA/CDraw.h>

import Legacy;
import Draw;
bool bUsingLegacy = false;

void Init()
{
    CIniReader reader(L"GTA3.WidescreenFixLegacy.ini");
    if (std::filesystem::exists(reader.GetIniPath()))
    {
        bUsingLegacy = true;
        InitLegacy();
        return;
    }

    auto pattern = hook::pattern("BE ? ? ? ? 59 89 C7");
    static auto WinMainHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onInitEvent().executeAll();
    });

    pattern = hook::pattern("8D 44 24 ? 6A ? 6A ? 6A ? 6A ? 50 FF 15 ? ? ? ? 85 C0 74 ? 83 7C 24");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static std::once_flag of;
        std::call_once(of, []()
        {
            WFP::onGameInitEvent().executeAll();
        });

        WFP::onGameProcessEvent().executeAll();
    });

    pattern = find_pattern("E8 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? 50 ? ? FF 51 ? 3D", "E8 ? ? ? ? 83 C4 ? A1 ? ? ? ? C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? 8B D8");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = find_pattern("8B 08 53 56 57 50 FF 91", "A1 ? ? ? ? ? ? 50 FF 92 ? ? ? ? A1");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });
}

CEXP void UpdateVars()
{
    if (bUsingLegacy)
    {
        LegacyUpdateVars();
    }
}

CEXP void __cdecl GetCurrentFOV(float* out)
{
    if (bUsingLegacy)
        LegacyGetCurrentFOV(out);
    else
        FOVManager::GetCurrentFOV(out);
}

CEXP void __cdecl SetFOVMultiplier(void* hash, float value)
{
    if (bUsingLegacy)
        LegacySetFOVMultiplier(hash, value);
    else
        FOVManager::SetFOVMultiplier(hash, value);
}

CEXP void __cdecl RemoveFOVMultiplier(void* hash)
{
    if (bUsingLegacy)
        LegacyRemoveFOVMultiplier(hash);
    else
        FOVManager::RemoveFOVMultiplier(hash);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("6A 02 6A 00 6A 00 68 01 20 00 00"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}
