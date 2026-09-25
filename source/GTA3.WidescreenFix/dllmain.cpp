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

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 52 ? 3B C3", "C7 44 24 ? ? ? ? ? 8B 10 89 04 24 FF 52 ? 85 C0 7C ? E8 ? ? ? ? 85 C0 74 ? 8B 85 ? ? ? ? 8B 95 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? E9 A4 01 00 00");
    static auto _rwD3D8CameraClearBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? 3D", "C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? 8B D8 81 FB");
    static auto _rwD3D8BeginSceneBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? 3B C3", "C7 44 24 ? ? ? ? ? 8B 10 89 04 24 FF 52 ? 85 C0 7C ? E8 ? ? ? ? 85 C0 74 ? 8B 85 ? ? ? ? 8B 95 ? ? ? ? A3 ? ? ? ? 89 15 ? ? ? ? E9 A7 01 00 00");
    static auto _rwD3D8CameraBeginUpdateBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? 8B F0");
    if (!pattern.empty())
    {
        static auto _rwD3D8CameraBeginUpdateBeforeResetHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });
    }

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 52 ? A1", "C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? E8 ? ? ? ? A1");
    static auto _rwD3D8RasterShowRasterBeforeResetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

    pattern = find_pattern("68 ? ? ? ? 50 ? ? FF 51 ? A1", "C7 44 24 ? ? ? ? ? ? ? ? ? ? FF 52 ? E8 ? ? ? ? 33 C0");
    static auto _rwD3D8RasterShowRasterBeforeResetHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs) { WFP::onBeforeReset().executeAll(); });

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

CEXP BOOL __cdecl RegisterBeforeResetCallback(void (__cdecl* callback)())
{
    if (!callback || bUsingLegacy)
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
