module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"
#include "include/WindowStyle.hpp"

export module Window;

import ComVars;
import Images;

// Window Mode: 0 the game's exclusive fullscreen, 1 windowed (a framed window whose client area has the
// resolution's size), 2 borderless (no frame). Both are centred on their monitor; at the desktop's size the client
// area covers the monitor (a frame lies outside the screen).
export namespace GTA2Window
{
    SafetyHookInline colorDepthHook;
    SafetyHookInline screenSettingHook;
    constexpr UINT ApplyWindowModeMessage = WM_APP + 0x5A;

    int __stdcall ReadScreenSetting(const char* name, int fallback)
    {
        auto value = screenSettingHook.stdcall<int>(name, fallback);
        if (strcmp(name, "start_mode") == 0) return windowedMode ? 0 : 1;
        return value;
    }

    void __cdecl SetColourDepth()
    {
        // WM_SETFOCUS can arrive inside DirectDraw mode creation, before gbh_InitDLL.
        if (bRendererReady) colorDepthHook.call<void>();
    }

    std::pair<int, int> OutputResolution()
    {
        if (windowMode == 2)
        {
            MONITORINFO monitor{ sizeof(monitor) };
            const auto display = InputDevices::Window ? MonitorFromWindow(InputDevices::Window, MONITOR_DEFAULTTONEAREST) :
                MonitorFromPoint(POINT{}, MONITOR_DEFAULTTOPRIMARY);
            if (GetMonitorInfoW(display, &monitor))
                return { monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top };
        }
        return { nResX, nResY };
    }

    void ApplyWindowMode(HWND window)
    {
        const int requested = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        if (requested == windowMode || !ReloadVideoMode)
            return;

        windowMode = requested;
        windowedMode = windowMode != 0;
        PatchWindowMode();
        (void)window;
        if (fullscreenWidth.is_initialized()) fullscreenWidth = nResX;
        if (fullscreenHeight.is_initialized()) fullscreenHeight = nResY;
        ReloadVideoMode();
    }

    void Install()
    {
        SettingsReader iniReader("");
        auto versionProbe = hook::get_pattern("50 51 E8 ? ? ? ? 81 7C 24 74 01 06 00 00 73 2E", 2);
        injector::MakeCALL(versionProbe, GTA2Images::DetectDirectX);
        bRendererReady.SetAddress(*hook::pattern("83 3D ? ? ? ? 00 75 04 33 C0 EB 17 C7 05").get_first<uint32_t*>(2));
        colorDepthHook = safetyhook::create_inline(hook::get_pattern("FF 15 ? ? ? ? E9 25 FF FF FF"), SetColourDepth);
        windowMode = std::clamp(iniReader.ReadInteger("MAIN", "WindowMode", 0), 0, 2);
        windowedMode = windowMode != 0;
        // Several registry readers share a prologue; take the one the screen setup calls for start_mode.
        auto screenSetting = hook::get_pattern("6A 01 68 ? ? ? ? B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 8B 0D", 17);
        screenSettingHook = safetyhook::create_inline(injector::GetBranchDestination(screenSetting).get<void>(), ReadScreenSetting);
        {
            // The bundled DirectDraw wrapper supplies the legacy pixel format.
            auto call = hook::get_pattern("83 EC 20 E8 ? ? ? ? 84 C0 75 04 83 C4 20 C3", 3);
            auto windowAllowed = injector::GetBranchDestination(call).get();
            static ProtectedGameRef<std::array<uint8_t, 3>> allowed;
            allowed.SetAddress(windowAllowed.get<std::array<uint8_t, 3>>());
            static const auto originalAllowed = allowed.get();
            // The renderer's windowed capability is also checked on focus loss, where
            // a "no" minimizes the window (alt-tab, clicking another window).
            auto windowCapable = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 50 51 FF 15 ? ? ? ? 85 C0 74 09 F6");
            static ProtectedGameRef<std::array<uint8_t, 3>> capable;
            capable.SetAddress(windowCapable.get_first<std::array<uint8_t, 3>>());
            static const auto originalCapable = capable.get();

            // Borderless: the windowed mode uses WS_OVERLAPPEDWINDOW; use a popup.
            auto style = hook::pattern("68 00 00 CF 10 6A F0 50 FF 15");
            static ProtectedGameRef<uint32_t> windowStyle;
            windowStyle.SetAddress(style.get_first<uint32_t>(1));
            static const auto originalStyle = windowStyle.get();
            PatchWindowMode = []
            {
                const std::array<uint8_t, 3> allowedWindow{ 0xB0, 0x01, 0xC3 };
                allowed = windowedMode ? allowedWindow : originalAllowed;
                capable = windowedMode ? allowedWindow : originalCapable;
                windowStyle = windowMode == 2 ? WS_POPUP | WS_VISIBLE : originalStyle;
            };
            PatchWindowMode();
            FixSettings().OnChange("MAIN.WindowMode", []
            {
                if (InputDevices::Window)
                    PostMessageW(InputDevices::Window, ApplyWindowModeMessage, 0, 0);
            });
            // After the game sized its window: centre it on its monitor, the client area at the resolution's size (at
            // the desktop's size it covers the monitor, the frame outside), no rounded corners.
            auto position = hook::pattern("8B 0D ? ? ? ? 03 C2 8B 15 ? ? ? ? 50 A1 ? ? ? ? 51 52 6A 00 50");
            auto clientWidth = hook::pattern("A1 ? ? ? ? 03 D1 8B 4C 24 18");
            auto clientHeight = hook::pattern("8B 15 ? ? ? ? 68 16 03 00 00");
            {
                hWnd.SetAddress(*position.get_first<HWND*>(16));
                nClientWidth.SetAddress(*clientWidth.get_first<int*>(1));
                nClientHeight.SetAddress(*clientHeight.get_first<int*>(2));
                static SafetyHookInline windowHook;
                windowHook = safetyhook::create_inline(static_cast<uint8_t*>(call) - 3, static_cast<char(__cdecl*)()>([]() -> char
                {
                    auto result = windowHook.ccall<char>();
                    if (result && hWnd)
                    {
                        if (windowedMode)
                            WindowStyle::Place(hWnd, nClientWidth, nClientHeight);
                        WindowStyle::SquareCorners(hWnd);
                    }
                    return result;
                }));
            }
        }
    }
}
