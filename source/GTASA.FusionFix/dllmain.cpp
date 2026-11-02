#include "stdafx.h"
#include "FixSettings.hpp"
#include "automation/asi_automation_frames.h"
#include <d3d9.h>
#include "Shared/WidescreenFixConflict.hpp"

import Draw;
import MenuConstraint;
import Skeleton;

// The settings' runtime interface for menu hosts and editors
extern "C" __declspec(dllexport) const AsiSettingsApi* ASI_SETTINGS_CALL AsiSettings_GetApi(uint32_t hostVersion)
{
    (void)hostVersion;
    return FixSettings().Api();
}

// Frame stages for the development-only automation plugin (inert unless it registers)
ASI_AUTOMATION_FRAMES_EXPORT()

void Init()
{
    FixSettings();
    asi_settings::LiveLink::Start();

    auto pattern = hook::pattern("BF ? ? ? ? 83 C4 ? 3B EF 89 44 24");
    static auto WinMainHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        WFP::onInitEvent().executeAll();
    });

    pattern = hook::pattern("8D 4C 24 ? 51 FF 15 ? ? ? ? 85 C0");
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
        if (asi_automation_frames::Wanted(ASI_AUTOMATION_STAGE_AFTER_UI) && pD3D9Device && *pD3D9Device)
            asi_automation_frames::Fire(ASI_AUTOMATION_STAGE_AFTER_UI, *pD3D9Device);
    });
}

CEXP void __cdecl GetCurrentFOV(float* out)
{
    FOVManager::GetCurrentFOV(out);
}

CEXP void __cdecl SetFOVMultiplier(void* hash, float value)
{
    FOVManager::SetFOVMultiplier(hash, value);
}

CEXP void __cdecl RemoveFOVMultiplier(void* hash)
{
    FOVManager::RemoveFOVMultiplier(hash);
}

CEXP BOOL __cdecl RegisterBeforeResetCallback(void (__cdecl* callback)())
{
    if (!callback)
        return FALSE;

    static std::vector<void (__cdecl*)()> callbacks;
    if (std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end())
        return TRUE;

    // The event retains the function pointer for the lifetime of the game.
    HMODULE callbackModule = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(callback), &callbackModule))
        return FALSE;

    callbacks.push_back(callback);
    WFP::onBeforeReset() += [callback]() { callback(); };
    return TRUE;
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("BF ? ? ? ? 83 C4 ? 3B EF 89 44 24"));
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
