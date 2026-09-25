module;
#include "stdafx.h"
#include "RTTI.h"

export module Windowed;
import ComVars;

namespace
{
    int mode = 0;
    HWND* gameWindow;
    SafetyHookInline settingsHook;
    SafetyHookInline displayHook;

    MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info);
        return info;
    }

    int __cdecl BuildSettings(void* settings)
    {
        auto result = settingsHook.unsafe_ccall<int>(settings);
        Game::Field<int>(settings, 0x18) = 1; // Native display mode: windowed.
        if (mode == 2)
        {
            const auto info = Monitor(*gameWindow);
            Game::Field<int>(settings, 0) = info.rcMonitor.right - info.rcMonitor.left;
            Game::Field<int>(settings, 4) = info.rcMonitor.bottom - info.rcMonitor.top;
        }
        return result;
    }

    BOOL __fastcall ApplyDisplay(void* display, void*, const void* requested)
    {
        std::array<uint32_t, 8> settings;
        memcpy(settings.data(), requested, sizeof(settings));
        settings[6] = 1;
        auto window = Game::Field<HWND>(display, 0x1C8);
        const auto info = Monitor(window);
        if (mode == 2)
        {
            settings[0] = info.rcMonitor.right - info.rcMonitor.left;
            settings[1] = info.rcMonitor.bottom - info.rcMonitor.top;
        }
        else
        {
            settings[0] = std::max(settings[0], 640u);
            settings[1] = std::max(settings[1], 480u);
        }

        // Fixed-size window: the game's resolution menu remains responsible for resizing.
        const DWORD style = WS_POPUP;
        const auto oldStyle = GetWindowLongW(window, GWL_STYLE);
        const auto newStyle = style | (oldStyle & WS_VISIBLE);
        const auto exStyle = GetWindowLongW(window, GWL_EXSTYLE);
        RECT rect{ 0, 0, static_cast<LONG>(settings[0]), static_cast<LONG>(settings[1]) };
        AdjustWindowRectEx(&rect, newStyle, FALSE, exStyle);
        const auto width = rect.right - rect.left;
        const auto height = rect.bottom - rect.top;
        const auto x = mode == 2 ? info.rcMonitor.left : info.rcWork.left + (info.rcWork.right - info.rcWork.left - width) / 2;
        const auto y = mode == 2 ? info.rcMonitor.top : info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - height) / 2;

        // WM_SIZE normally re-enters the reset path. Use the engine's own mode-change guard.
        auto& changingMode = Game::Field<uint8_t>(display, 0x1CF);
        const auto previousGuard = changingMode;
        changingMode = 1;
        SetWindowLongW(window, GWL_STYLE, newStyle);
        Game::Field<LONG>(display, 0x1F8) = newStyle;
        SetWindowPos(window, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        changingMode = previousGuard;

        // The native windowed branch selects desktop format and zero fullscreen refresh.
        const auto result = displayHook.unsafe_fastcall<BOOL>(display, nullptr, settings.data());
        changingMode = 1;
        SetWindowPos(window, HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        changingMode = previousGuard;
        return result;
    }
}

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            CIniReader iniReader("");
            auto nWindowedMode = std::clamp(iniReader.ReadInteger("MAIN", "WindowedMode", 0), 0, 2);

            mode = nWindowedMode;
            if (!mode) return;
            const auto displayVtable = ScarfaceRTTI::FindVtable(".?AVd3dDisplay@pure3d@@");
            if (!displayVtable) return;

            auto pattern = hook::pattern("A3 ? ? ? ? 5B 74");
            gameWindow = *pattern.get_first<HWND*>(1);

            pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? 8D 54 24 ? 52 E8");
            settingsHook = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BuildSettings);

            // d3dDisplay's display-settings method occupies vtable slot 10.
            const auto displaySlot = displayVtable + 10 * sizeof(void*);
            displayHook = safetyhook::create_inline(injector::ReadMemory<void*>(displaySlot, true), ApplyDisplay);
        };
    }
} Windowed;
