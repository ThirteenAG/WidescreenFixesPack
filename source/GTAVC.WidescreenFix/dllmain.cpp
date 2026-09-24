#include "stdafx.h"
#include <GTA/CDraw.h>

import Legacy;
import Draw;
bool bUsingLegacy = false;

void Init()
{
    CIniReader reader(L"GTAVC.WidescreenFixLegacy.ini");
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

    pattern = find_pattern<3>("68 ? ? ? ? 50 ? ? FF 52", "68 ? ? ? ? 50 ? ? FF 52");
    static auto _rwD3D8CameraClearBeforeResetHook = safetyhook::create_mid(pattern.get(0).get<void*>(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });
    //static auto _rwD3D8BeginSceneBeforeResetHook = safetyhook::create_mid(pattern.get(1).get<void*>(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });
    static auto _rwD3D8CameraBeginUpdateBeforeResetHook = safetyhook::create_mid(pattern.get(2).get<void*>(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? 3D");
    static auto _rwD3D8BeginSceneBeforeResetHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? A1 ? ? ? ? 68 ? ? ? ? 50 ? ? FF 92 ? ? ? ? A1 ? ? ? ? 50 ? ? FF 51 ? A1 ? ? ? ? 68 ? ? ? ? 50 ? ? FF 92 ? ? ? ? A1 ? ? ? ? 50 ? ? FF 51 ? E8 ? ? ? ? 85 C0 0F 84");
    static auto _rwD3D8RasterShowRasterBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? A1 ? ? ? ? 68 ? ? ? ? 50 ? ? FF 92 ? ? ? ? A1 ? ? ? ? 50 ? ? FF 51 ? A1 ? ? ? ? 68 ? ? ? ? 50 ? ? FF 92 ? ? ? ? A1 ? ? ? ? 50 ? ? FF 51 ? E8 ? ? ? ? 85 C0 74");
    static auto _rwD3D8RasterShowRasterBeforeResetHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("A1 ? ? ? ? 50 ? ? FF 91 ? ? ? ? 89 2D");
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
    else if (reason == DLL_PROCESS_DETACH)
    {
        WFP::onShutdownEvent().executeAll();
    }
    return TRUE;
}
