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

    Movie::Init({ "FF 15 ? ? ? ? 8B 44 24 0C 8B 54 24 08 50 8B 44 24 08 8B 4C 24 04 52 50 A1 ? ? ? ? 8B 18 51 50 FF 93 9C 00 00 00" });

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

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 52 ? 3B C3", "C7 44 24 ? ? ? ? ? 8B 10 89 04 24 FF 52 ? 85 C0 7C ? E8 ? ? ? ? 85 C0 74 ? 8B 85 ? ? ? ? 8B 95 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? E9 A4 01 00 00");
    static auto _rwD3D8CameraClearBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? 3D", "C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? 8B D8 81 FB");
    static auto _rwD3D8BeginSceneBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = hook::pattern("68 ? ? ? ? 50 ? ? FF 51 ? 3B C3");
    if (!pattern.empty()) // Original executable; the alternate build has one camera reset site.
    {
        static auto cameraBeginUpdate = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });
        auto second = hook::pattern("68 ? ? ? ? 50 ? ? FF 51 ? 8B F0");
        static auto cameraBeginUpdate2 = safetyhook::create_mid(second.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });
    }
    else
    {
        auto alternate = hook::pattern("C7 44 24 ? ? ? ? ? 8B 10 89 04 24 FF 52 ? 85 C0 7C ? E8 ? ? ? ? 85 C0 74 ? 8B 85 ? ? ? ? 8B 95 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? E9 A7 01 00 00");
        static auto cameraBeginUpdate = safetyhook::create_mid(alternate.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });
    }

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 52 ? A1", "C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? E8 ? ? ? ? A1");
    static auto _rwD3D8RasterShowRasterBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? A1", "C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? E8 ? ? ? ? 33 C0");
    static auto _rwD3D8RasterShowRasterBeforeResetHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    // Run at presentation, as in SA, rather than CameraEndUpdate: frontend
    // transitions also end camera updates while capturing offscreen textures.
    pattern = find_pattern("8B B0 98 00 00 00 E8 ? ? ? ? 8B 4C 24 14 8B 54 24 10 8B 7C 24 0C 51 52 57 FF D6", "8B 98 98 00 00 00 E8 ? ? ? ? 8B 44 24 1C 8B 54 24 20 8B CB 8B 5C 24 18");
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
