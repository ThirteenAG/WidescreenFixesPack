#include "stdafx.h"
#include "FixSettings.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/WidescreenFixConflict.hpp"
#include "rawinput.hpp"

import ComVars;

// The settings' runtime interface for menu hosts and editors (AsiSettings_GetApi)
ASI_SETTINGS_EXPORT(FixSettings())

namespace
{
    decltype(&PeekMessageA) NativePeekMessage = nullptr;
    thread_local int MovieQuitCode = 0;

    BOOL WINAPI PeekGameMessage(LPMSG message, HWND filter, UINT first, UINT last, UINT flags)
    {
        for (;;)
        {
            if (NativePeekMessage(message, filter, first, last, flags))
                return TRUE;

            const auto window = SHDisplay::Get().window;
            if (!window || !IsWindow(window) || GetForegroundWindow() == window ||
                FixSettings().Get("MAIN.PauseOnFocusLoss") != "1")
                return FALSE;

            MsgWaitForMultipleObjectsEx(0, nullptr, 50, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
        }
    }

    BOOL WINAPI PeekMovieMessage(LPMSG message, HWND filter, UINT first, UINT last, UINT flags)
    {
        const auto available = PeekGameMessage(message, filter, first, last, flags);
        if (available && message->message == WM_QUIT)
            MovieQuitCode = static_cast<int>(message->wParam);
        return available;
    }

    BOOL WINAPI PeekMainMessage(LPMSG message, HWND filter, UINT first, UINT last, UINT flags)
    {
        const auto available = PeekGameMessage(message, filter, first, last, flags);
        if (available && message->message == WM_QUIT)
            RWGTA::UpdateCheck::OnExit();
        return available;
    }

    void WINAPI RepostMovieQuit(int)
    {
        PostQuitMessage(std::exchange(MovieQuitCode, 0));
    }

    void InstallGameMessagePumps()
    {
        MessagePumpGuard::Install(GetModuleHandleW(nullptr));
        auto game = hook::pattern("6A 01 56 56 56 8D 55 80 52 FF 15 ? ? ? ? 85 C0 74 32 83 EC 1C B9 07 00 00 00");
        auto movie = hook::pattern("8B 3D ? ? ? ? 8B 1D ? ? ? ? 8B 2D ? ? ? ? 83 7C 24 14 12 0F 84");
        auto movieQuit = hook::pattern("50 E8 ? ? ? ? 83 C4 04 56 FF 15 ? ? ? ? 5F 5E 5D 33 C0 5B 83 C4 1C C3");

        // The native loops keep owning their message dispatch and movie cleanup.
        NativePeekMessage = **game.get_first<decltype(&PeekMessageA)*>(11);
        static const auto gamePeek = &PeekMainMessage;
        static const auto moviePeek = &PeekMovieMessage;
        static const auto postQuit = &RepostMovieQuit;
        injector::WriteMemory(game.get_first(11), &gamePeek, true);
        injector::WriteMemory(movie.get_first(2), &moviePeek, true);
        injector::WriteMemory(movieQuit.get_first(11), &postQuit, true);
    }
}

void Init()
{
    SetCurrentDirectoryW(GetExeModulePath<std::filesystem::path>().c_str());
    // Settings described in settings.json and stored in the plugin's .cfg; FusionFixHub changes them while the game
    // runs through the live link, served for all plugins of the game
    FixSettings();
    asi_settings::LiveLink::Start();
    const auto confineCursor = []()
    {
        CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
    };
    confineCursor();
    FixSettings().OnChange("MAIN.ConfineCursor", confineCursor);

    const auto windowMode = FixSettingNumber("MAIN.WindowMode", 0);
    SHDisplay::Install(windowMode == 0 ? SHDisplay::Mode::Fullscreen :
        (windowMode == 1 ? SHDisplay::Mode::Windowed : SHDisplay::Mode::Borderless));
    InstallGameMessagePumps();

    const auto anisotropy = []()
    {
        SHDisplay::SetAnisotropy(FixSettingNumber("GRAPHICS.AnisotropicFiltering", 0));
    };
    anisotropy();
    FixSettings().OnChange("GRAPHICS.AnisotropicFiltering", anisotropy);

    auto reset = hook::pattern("83 EC 48 53 33 DB 33 C0 57 B9 0C 00 00 00 8D 7C 24 20 89 5C 24 1C F3 AB");
    auto fullscreen = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 89 44 24 1C 39 1D ? ? ? ? BF 01 00 00 00");
    static auto nativeReset = reinterpret_cast<HRESULT(__cdecl*)(int)>(reset.get_first());
    bFullscreen.SetAddress(*fullscreen.get_first<int32_t*>(17));
    // Keep the game's device mode and foreground checks in sync with the actual window mode.
    auto createMode = hook::pattern("89 4C 24 18 39 35 ? ? ? ? 89 15 ? ? ? ? 8B 15 ? ? ? ? A3");
    static SafetyHookMid CreateModeHook = safetyhook::create_mid(createMode.get_first(4), [](SafetyHookContext&)
    {
        bFullscreen = SHDisplay::Get().mode == SHDisplay::Mode::Fullscreen ? 1 : 0;
    });
    auto resolutionMode = hook::pattern("6A 01 C7 05 ? ? ? ? 01 00 00 00 E8 ? ? ? ? 83 C4 04 85 C0 7D");
    injector::MakeNOP(resolutionMode.get_first(2), 10, true);
    SHDisplay::Detail::NativeModeReset = [](SHDisplay::Mode mode)
    {
        bFullscreen = mode == SHDisplay::Mode::Fullscreen ? 1 : 0;
        return nativeReset(1);
    };

    // Game state the camera and the widescreen code both read, bound before the widescreen code hooks the camera
    // code it is read from
    aPlayerStatus.SetAddress(*hook::pattern("55 8B EC 83 E4 F0 83 EC 14 8B 0D ? ? ? ? A1 ? ? ? ? 8B 15 ? ? ? ? 53 56 89 0D").get(0).get<uint8_t*>(0x10));
    aMoveIntent.SetAddress(*hook::pattern("B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 8B 15").get(0).get<float*>(1));
    auto gameCamera = hook::pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 10 E9");
    aCameraMatrix.SetAddress(*gameCamera.get(0).get<float*>(1));
    fCameraFov.SetAddress(*gameCamera.get(0).get<float*>(0x1A));
    ApplyCamera = reinterpret_cast<decltype(ApplyCamera)>(gameCamera.get(0).get<void>(0));
    nGameMode.SetAddress(*hook::pattern("A1 ? ? ? ? 8B 34 85 ? ? ? ? 33 FF").get(0).get<int32_t*>(1));
    nSceneFlags.SetAddress(*hook::pattern("A1 ? ? ? ? 83 E0 01 C3 90 90 90 90 90 90 90 55 8B EC 83 EC 1C").get(0).get<int32_t*>(1));

    // System, Camera, Input, Widescreen and Option 4 (Advanced Options) of the Options screen (Menu)
    WFP::onInitEvent().executeAll();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        if (WidescreenFixConflict::Found())
            return;
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8B 0D ? ? ? ? 6A 00 50 51"));
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
