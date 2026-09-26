module;

#include "stdafx.h"

export module Windowed;

import Screen;

namespace
{
    int nWindowedMode = 0;
    int32_t* pWidth = nullptr;
    int32_t* pHeight = nullptr;
    injector::hook_back<HWND(__cdecl*)(LPCSTR, LPCSTR, int, int, LONG, LONG, WNDPROC, HINSTANCE, int, DWORD)> hbCreateWindow;

    MONITORINFO PrimaryMonitor()
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    void UpdateResolution()
    {
        const auto info = PrimaryMonitor();
        if (nWindowedMode == 2)
        {
            *pWidth = info.rcMonitor.right - info.rcMonitor.left;
            *pHeight = info.rcMonitor.bottom - info.rcMonitor.top;
        }
        Screen.Update(*pWidth, *pHeight);
    }

    BOOL WINAPI PositionWindow(HWND window, HWND, int, int, int, int, UINT)
    {
        const auto info = PrimaryMonitor();
        const DWORD style = WS_POPUP;
        const auto visible = GetWindowLongW(window, GWL_STYLE) & WS_VISIBLE;
        SetWindowLongW(window, GWL_STYLE, style | visible);
        const auto exStyle = GetWindowLongW(window, GWL_EXSTYLE) & ~WS_EX_TOPMOST;
        SetWindowLongW(window, GWL_EXSTYLE, exStyle);

        RECT rect{ 0, 0, *pWidth, *pHeight };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);
        const auto width = rect.right - rect.left;
        const auto height = rect.bottom - rect.top;
        // Desktop-sized windows must cover the monitor exactly for the shell to
        // hide the taskbar. Center in the work area only when the window fits.
        const auto& area = nWindowedMode == 2 || width > info.rcWork.right - info.rcWork.left || height > info.rcWork.bottom - info.rcWork.top
            ? info.rcMonitor : info.rcWork;
        const auto x = area.left + (area.right - area.left - width) / 2;
        const auto y = area.top + (area.bottom - area.top - height) / 2;
        return SetWindowPos(window, HWND_NOTOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    HWND __cdecl CreateGameWindow(LPCSTR className, LPCSTR title, int, int, LONG width, LONG height, WNDPROC wndProc, HINSTANCE instance, int icon, DWORD)
    {
        // Use the native windowed creation branch instead of a topmost popup.
        auto window = hbCreateWindow.fun(className, title, -1, -1, width, height, wndProc, instance, icon, 0);
        if (window)
            PositionWindow(window, nullptr, 0, 0, 0, 0, 0);
        return window;
    }
}

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []
        {
            CIniReader iniReader("");
            nWindowedMode = std::clamp(iniReader.ReadInteger("MAIN", "WindowedMode", 0), 0, 2);
            if (!nWindowedMode)
                return;

            auto pattern = hook::pattern("A3 ? ? ? ? FF D6 50"); // 0x5AC3F9 + 1
            pWidth = *pattern.get_first<int32_t*>(1);
            pattern = hook::pattern("DB 05 ? ? ? ? 83 C4 18 6A 00 D8 0D"); // 0x5AC40E + 2
            pHeight = *pattern.get_first<int32_t*>(2);
            static auto ResolutionHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                UpdateResolution();
            });

            // Select native windowed D3D presentation before renderer creation.
            pattern = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 FF 15 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 6A 04"); // 0x5AC4C7 + 6
            injector::WriteMemory<uint32_t>(pattern.get_first(6), 1, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 28 6A 03 51"); // 0x5AC4A9
            hbCreateWindow.fun = injector::MakeCALL(pattern.get_first(), CreateGameWindow, true).get();
            // Show normally, without maximizing the selected resolution.
            injector::WriteMemory<uint8_t>(pattern.get_first(15), SW_SHOWNORMAL, true); // 0x5AC4A9 + 15

            // Replace only these two six-byte imported calls. The NGL call also
            // adds a pixel to each dimension, which must not resize our client area.
            pattern = hook::pattern("FF 15 ? ? ? ? 8B 3D ? ? ? ? 57 E8 ? ? ? ? 83 C4 04 6A 00"); // 0x5AC4F3
            injector::MakeNOP(pattern.get_first(), 6, true);
            injector::MakeCALL(pattern.get_first(), PositionWindow, true);
            pattern = hook::pattern("FF 15 ? ? ? ? EB 3A 68 ? ? ? ? 50"); // 0x76D803
            injector::MakeNOP(pattern.get_first(), 6, true);
            injector::MakeCALL(pattern.get_first(), PositionWindow, true);
        };
    }
} Windowed;
