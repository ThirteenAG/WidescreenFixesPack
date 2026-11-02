#include "stdafx.h"
#include "FixSettings.hpp"
#include "automation/asi_automation_frames.h"
#include <d3d9.h>
#include "Shared/WidescreenFixConflict.hpp"

import Draw;
import MenuConstraint;
import Movie;
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

    Movie::Init({
        "FF 15 ? ? ? ? 8B 84 24 0C 02 00 00 8B 8C 24 08 02 00 00 50 8B 84 24 08 02 00 00 8B 94 24 04 02 00 00 51 50 A1 ? ? ? ? 8B 18 52 50 FF 93 9C 00 00 00",
        "FF 15 ? ? ? ? 8B 44 24 ? 8B 5C 24 ? 50 8B 44 24 ? 8B 4C 24 ? 53 50 A1 ? ? ? ? 8B 28 51 50 FF 95 9C 00 00 00"
    });

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

    // Execute once at presentation, including frames whose camera update has
    // already ended. Offscreen frontend captures must not clear the AA border.
    pattern = hook::pattern("8B B0 98 00 00 00 E8 ? ? ? ? 8B 4C 24 14 8B 54 24 10 8B 7C 24 0C 51 52 57 FF D6");

    {
        static auto BeforeRasterShowRasterHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            WFP::onEndScene().executeAll();
            if (asi_automation_frames::Wanted(ASI_AUTOMATION_STAGE_AFTER_UI) && pD3D8Device && *pD3D8Device)
            {
                IDirect3DDevice9* device = nullptr;
                if (SUCCEEDED((*pD3D8Device)->QueryInterface(__uuidof(IDirect3DDevice9), reinterpret_cast<void**>(&device))))
                {
                    asi_automation_frames::Fire(ASI_AUTOMATION_STAGE_AFTER_UI, device);
                    device->Release();
                }
            }
        });
    }
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
