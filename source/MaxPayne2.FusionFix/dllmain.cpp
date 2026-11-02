#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/MaxPayne/GamepadButtons.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#include "Shared/WidescreenFixConflict.hpp"

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import x_basicmodesmfc;
import x_gameobjectsmfc;
import x_helpersmfc;
import x_modesmfc;
import sndmfc;
import x_inputmfc;
import xidi;
import PostFX;
import Window;
import Settings;
import SettingsFile;
import Menu;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

// The game; the per-frame event comes from the end of the frame (X_ModesMFC)
void Init()
{
    auto& settings = FixSettings();
    auto language = settings.String("MAIN.TextLanguage");
    if (!language.empty() && language != "auto")
        settings.GetTexts().Select(language);

    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        D3D8Compatibility::Initialize(true);
        // Per monitor DPI awareness, so the game gets real pixels on scaled desktops. The process' awareness can be set
        // only once: when it's set already (the exe's compatibility settings, "Override high DPI scaling", make it per
        // monitor v1 or system aware before any plugin loads), the windows this thread creates (the game's) get it
        using SetContext = BOOL(WINAPI*)(HANDLE);
        using SetThreadContext = HANDLE(WINAPI*)(HANDLE);
        auto user32 = GetModuleHandleW(L"user32.dll");
        if (auto set = reinterpret_cast<SetContext>(GetProcAddress(user32, "SetProcessDpiAwarenessContext")))
        {
            if (!set(reinterpret_cast<HANDLE>(-4)))
            {
                if (auto setThread = reinterpret_cast<SetThreadContext>(GetProcAddress(user32, "SetThreadDpiAwarenessContext")))
                    setThread(reinterpret_cast<HANDLE>(-4));
            }
        }
        else
            SetProcessDPIAware();

        // Settings described in settings.json and stored in the plugin's .cfg; editors change them through the live link
        FixSettings();
        WindowMode::nWindowMode = std::clamp(FixSettings().Int("MAIN.WindowMode"), (int32_t)WindowMode::Fullscreen, (int32_t)WindowMode::Borderless);
        asi_settings::LiveLink::Start();
        ReadSettings();
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []() { CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor")); });
        InitSettingsFile();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 40 04 68"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", []()
        {
            InitE2MFC();
            auto e2 = GetModuleHandleW(L"E2MFC.dll");
            HMODULE host = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&InitMenu), &host);
            MaxPayneIcons::FontLayout fontLayout{ 0x22, 0x44, 0x2454, L"msvcr71.dll" };
            MaxPayneIcons::Init(host, e2, GetModuleHandleW(L"rlmfc.dll"),
                []() { return FixSettings().Int("MAIN.GamepadIcons"); }, MaxPayneGamepadButtons::IsPadInUse, fontLayout);
        });
        CallbackHandler::RegisterCallback(L"X_GameObjectsMFC.dll", InitX_GameObjectsMFC);
        CallbackHandler::RegisterCallback(L"X_ModesMFC.dll", InitX_ModesMFC);
        CallbackHandler::RegisterCallback(L"X_HelpersMFC.dll", InitX_HelpersMFC);
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { InitE2_D3D8_DRIVER_MFC(); InitPostFXDriver(); });
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { BorderlessWindowedHook.reset(); shDllMainHook.reset(); ShutdownPostFXDriver(); });
        CallbackHandler::RegisterCallback(L"X_BasicModesMFC.dll", []() { InitX_BasicModesMFC(); InitMenu(); });
        CallbackHandler::RegisterCallback(L"sndmfc.dll", InitSNDMFC);
        CallbackHandler::RegisterCallback(L"X_Inputmfc.dll", InitInput);
        CallbackHandler::RegisterCallback(L"Xidi.32.dll", InitXidi);
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
        CursorConfinement::Shutdown();
    }
    return TRUE;
}
