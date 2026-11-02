#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/D3D8Compatibility.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/MaxPayne/FrameRate.hpp"
#include "Shared/MaxPayne/NativeIcons.hpp"
#include "Shared/WidescreenFixConflict.hpp"
#include "DeveloperOverlay.hpp"

import ComVars;
import e2mfc;
import e2_d3d8_driver_mfc;
import input;
import xidi;
import PostFX;
import RawMouse;
import Window;
import Cinematic;
import Settings;
import SettingsFile;
import Video;
import Menu;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

CEXP int __cdecl AsiAutomation_SetFrameCallback(int stage, void(__cdecl* callback)(int, void*, void*), void* context)
{
    return RegisterAutomationFrameCallback(stage, callback, context);
}

void Init()
{
    auto& settings = FixSettings();
    auto language = settings.String("MAIN.TextLanguage");
    if (!language.empty() && language != "auto")
        settings.GetTexts().Select(language);

    WFP::onInitEvent().executeAll();

    // End of the frame, before P_Driver::endScene: the bars the Fusion Fix adds are drawn here with
    // P_Driver::clearScreen, the same way the game draws its letterbox bars
    auto pattern = hook::pattern("56 8B F1 8B 4E 0C FF 15 ? ? ? ? 8A 46 28 84 C0 C6 46 37 00");
    static auto FrameEndHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
    {
        Cinematic::DrawFrameBorders();
        RenderDeveloperOverlay();
        RenderAutomationFrame(1);
        WFP::onGameProcessEvent().executeAll();

        // Frame rate limit, before the frame is shown
        static auto& nFPSLimit = FixSettings().Int("MAIN.FPSLimit");
        auto window = WindowedModeWrapper::GameHWND;
        if (!window || !IsWindow(window))
        {
            window = GetRenderWindow();
            if (!window)
                window = GetActiveWindow();
            DWORD processId = 0;
            if (window && GetWindowThreadProcessId(window, &processId) && processId == GetCurrentProcessId())
                WindowedModeWrapper::GameHWND = window;
            else
                window = nullptr;
        }
        CursorConfinement::Suspend(MaxPayneOverlay::BlocksInput());
        CursorConfinement::Frame(window);
        UpdateRawMouseWindow(window);
        MaxPayneFrameRate::Limit(nFPSLimit);
    });
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        D3D8Compatibility::Initialize(true);
        // Per monitor DPI awareness, so the game gets real pixels on scaled desktops
        using SetContext = BOOL(WINAPI*)(HANDLE);
        using SetThreadContext = HANDLE(WINAPI*)(HANDLE);
        if (auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
        {
            if (!set(reinterpret_cast<HANDLE>(-4)))
            {
                if (auto setThread = reinterpret_cast<SetThreadContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext")))
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
        MaxPayneVideo::InitStartupWindow();
        MaxPayneVideo::Init();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("0F 84 ? ? ? ? E8 ? ? ? ? 8B 48 04 68 ? ? ? ? 56 89"));
        CallbackHandler::RegisterCallback(L"E2MFC.dll", []()
        {
            InitE2MFC();
            HMODULE host = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&InitMenu), &host);
            MaxPayneIcons::Init(host, GetModuleHandleW(L"E2MFC.dll"), GetModuleHandleW(L"rlmfc.dll"),
                []() { return FixSettings().Int("MAIN.GamepadIcons"); }, MaxPayneGamepadButtons::IsPadInUse);
            InitControlIcons();
        });
        CallbackHandler::RegisterCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { InitE2_D3D8_DRIVER_MFC(); InitPostFXDriver(); });
        CallbackHandler::RegisterModuleUnloadCallback(L"E2_D3D8_DRIVER_MFC.dll", []() { BorderlessWindowedHook.reset(); shDllMainHook.reset(); ShutdownPostFXDriver(); });
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
