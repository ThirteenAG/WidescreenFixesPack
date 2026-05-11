#include "stdafx.h"
#include <GTA/CDraw.h>

import Legacy;
import Draw;
bool bUsingLegacy = false;

void Init()
{
    CIniReader reader(L"GTASA.WidescreenFixLegacy.ini");
    if (std::filesystem::exists(reader.GetIniPath()))
    {
        bUsingLegacy = true;
        InitLegacy();
        return;
    }

    auto pattern = hook::pattern("BF ? ? ? ? 83 C4 ? 3B EF 89 44 24");
    static auto WinMainHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onInitEvent().executeAll();
    });

    pattern = hook::pattern("6A ? 53 53 53 8D 4C 24");
    static auto MainLoopHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        static std::once_flag of;
        std::call_once(of, []()
        {
            WFP::onGameInitEvent().executeAll();
        });

        WFP::onGameProcessEvent().executeAll();
    });

    pattern = find_pattern("89 1D ? ? ? ? 74 ? 33 F6 8B 86 ? ? ? ? 3B C3 74 ? 50 A1");
    static auto BeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onBeforeReset().executeAll();
    });

    pattern = find_pattern("A1 ? ? ? ? 50 ? ? FF 91 ? ? ? ? C7 05");
    static auto BeforeEndSceneHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onEndScene().executeAll();
    });
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
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("BF ? ? ? ? 83 C4 ? 3B EF 89 44 24"));
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
