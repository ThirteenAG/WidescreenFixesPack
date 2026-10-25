module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module WindowedMode;

import ComVars;

// The game's own window modes (WinDrv/D3DDrv):
// - UWindowsViewport::OpenWindow always sets UWindowsClient::StartupFullscreen (+D8h) to 1, the ini value is never used,
//   so the game can only start fullscreen. TryRenderDevice -> UD3DRenderDevice::SetRes(viewport, x, y, fullscreen)
//   makes the device (Windowed = !fullscreen, the desktop format, rate 0 in a window) and calls
//   UWindowsViewport::ResizeViewport(flags), which in a window sizes it with AdjustWindowRect and MoveWindow.
// - The game's MoveWindow keeps the window where it was (at CW_USEDEFAULT, or over the old display mode after fullscreen),
//   a window as big as the desktop gets its title bar and bottom off screen and is clamped by the max track size.
// - Alt+Enter only goes from a window to fullscreen (ToggleFullscreen/EndFullscreen aren't in this build), WM_SIZE sets the
//   resolution to the new client size, in fullscreen the window is minimized when it loses the focus.
// Here OpenWindow starts in the chosen mode, ResizeViewport places the window, Alt+Enter
// also goes back to the window mode and a borderless window renders at the monitor's resolution.
namespace WindowedMode
{
    enum { Fullscreen, Windowed, Borderless };

    int nFullScreenRefreshRateInHz = 0;

    HWND ViewportWindow(uintptr_t viewport)
    {
        auto window = viewport ? *reinterpret_cast<uintptr_t*>(viewport + 0x1F0) : 0;
        return window ? *reinterpret_cast<HWND*>(window + 4) : NULL;
    }

    MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info = { sizeof(info) };
        GetMonitorInfo(window ? MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    // The frame around the client area of the window with this style, at the DPI of its monitor
    RECT Frame(HWND window, LONG style, LONG exStyle)
    {
        using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
        using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        static auto user32 = GetModuleHandleW(L"user32.dll");
        static auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"));
        static auto adjustWindowRectExForDpi = reinterpret_cast<AdjustWindowRectExForDpiFn>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        RECT frame = {};
        if (getDpiForWindow && adjustWindowRectExForDpi)
            adjustWindowRectExForDpi(&frame, style, FALSE, exStyle, getDpiForWindow(window));
        else
            AdjustWindowRectEx(&frame, style, FALSE, exStyle);
        return frame;
    }

    // No rounded corners on Windows 11 (DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_DONOTROUND), nothing happens before it
    void SquareCorners(HWND window)
    {
        using SetAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttributeFn
        {
            auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return dwm ? reinterpret_cast<SetAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        int preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    // Borderless covers the monitor. A window is centered in the work area when it fits, otherwise its client area is centered
    // on the monitor and the frame goes past the edges (at the desktop resolution it covers the screen like borderless).
    void Place(HWND window, int width, int height)
    {
        // fullscreen minimizes the window when it isn't active (leaving it with Alt+Enter does that too)
        if (IsIconic(window))
            ShowWindow(window, SW_RESTORE);
        auto info = Monitor(window);
        auto style = GetWindowLong(window, GWL_STYLE) & ~(WS_MINIMIZE | WS_MAXIMIZE);
        auto exStyle = GetWindowLong(window, GWL_EXSTYLE);
        RECT rect = info.rcMonitor;
        if (bBorderless)
        {
            style = (style & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) | WS_POPUP;
            exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
        }
        else
        {
            style = (style & ~(WS_POPUP | WS_MAXIMIZEBOX)) | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
            auto frame = Frame(window, style, exStyle);
            auto& work = info.rcWork;
            auto& area = info.rcMonitor;
            auto w = width + (frame.right - frame.left);
            auto h = height + (frame.bottom - frame.top);
            if (w <= work.right - work.left && h <= work.bottom - work.top)
            {
                rect.left = work.left + ((work.right - work.left) - w) / 2;
                rect.top = work.top + ((work.bottom - work.top) - h) / 2;
            }
            else
            {
                rect.left = area.left + ((area.right - area.left) - width) / 2 + frame.left;
                rect.top = area.top + ((area.bottom - area.top) - height) / 2 + frame.top;
            }
            rect.right = rect.left + w;
            rect.bottom = rect.top + h;
        }
        SetWindowLong(window, GWL_STYLE, style);
        SetWindowLong(window, GWL_EXSTYLE, exStyle);
        SetWindowPos(window, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
        SquareCorners(window);
    }

    // UD3DRenderDevice::SetRes(viewport, width, height, fullscreen, a5): a borderless window renders at the monitor's resolution
    SafetyHookInline shSetRes = {};
    int __fastcall SetRes(uintptr_t device, void* edx, uintptr_t viewport, int width, int height, int fullscreen, int a5)
    {
        if (!fullscreen && bBorderless)
        {
            auto area = Monitor(ViewportWindow(viewport)).rcMonitor;
            width = area.right - area.left;
            height = area.bottom - area.top;
        }
        return shSetRes.unsafe_fastcall<int>(device, edx, viewport, width, height, fullscreen, a5);
    }
}

// Alt+Enter in fullscreen: the window mode at the windowed resolution (UWindowsClient +50h, +54h), as the game does it the other way
export void LeaveFullscreen(uintptr_t viewport)
{
    auto client = *reinterpret_cast<uintptr_t*>(viewport + 0x18);
    auto device = *reinterpret_cast<uintptr_t*>(viewport + 0x60);
    if (!client || !device)
        return;
    // the viewport setters (+288h, +290h) the game's Alt+Enter calls around SetRes
    auto vtable = *reinterpret_cast<uintptr_t**>(viewport);
    auto Set288 = reinterpret_cast<int(__thiscall*)(uintptr_t, int)>(vtable[192 / 4]);
    auto Set290 = reinterpret_cast<int(__thiscall*)(uintptr_t, int)>(vtable[208 / 4]);
    auto SetRes = reinterpret_cast<int(__thiscall*)(uintptr_t, uintptr_t, int, int, int, int)>((*reinterpret_cast<uintptr_t**>(device))[104 / 4]);
    Set288(viewport, 0);
    SetRes(device, viewport, *reinterpret_cast<int*>(client + 0x50), *reinterpret_cast<int*>(client + 0x54), 0, 0);
    Set290(viewport, 0);
    Set288(viewport, 1);
}

export void InitWindowedMode()
{
    using namespace WindowedMode;

    auto& settings = FixSettings();
    auto nWindowMode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
    bBorderless = nWindowMode == Borderless;
    nFullScreenRefreshRateInHz = settings.Int("WINDOWEDMODE.FullScreenRefreshRateInHz");

    // Per monitor DPI awareness, so a scaled desktop doesn't crop or blur the game
    using SetContextFn = BOOL(WINAPI*)(HANDLE);
    if (auto set = reinterpret_cast<SetContextFn>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
        set(reinterpret_cast<HANDLE>(-4)); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
    else
        SetProcessDPIAware();

    // OpenWindow: StartupFullscreen = 1 -> the chosen mode
    auto pattern = hook::pattern("C7 85 D8 00 00 00 01 00 00 00 39 1D");
    if (!pattern.empty())
        ProtectedGameRef<uint32_t>(reinterpret_cast<uint32_t*>(pattern.get_first(6))) = nWindowMode == Fullscreen ? 1 : 0;

    // ResizeViewport, window path: ecx = Viewport->Actor, its ShowFlags & SHOW_ChildWindow skip the MoveWindow.
    // The window is placed here (with the viewport's lock at +200h so WM_SIZE doesn't set the resolution again) and the game's
    // move is skipped (Actor isn't read).
    pattern = hook::pattern("FF 15 ? ? ? ? 8B 4E 30 8B 81 0C 04 00 00 F6 C4 02 75");
    if (!pattern.empty())
    {
        static auto ResizeViewportWindowHook = safetyhook::create_mid(pattern.get_first(9), [](SafetyHookContext& regs)
        {
            static uint8_t childWindowActor[0x410] = {};
            *reinterpret_cast<uint32_t*>(childWindowActor + 0x40C) = 0x200;
            regs.ecx = reinterpret_cast<uintptr_t>(childWindowActor);

            // WM_SIZE (the player resized the window): it stays where it is
            if (nViewportWndProcMsg == WM_SIZE)
                return;

            auto viewport = regs.esi;
            if (auto window = ViewportWindow(viewport))
            {
                ++*reinterpret_cast<int*>(viewport + 0x200);
                Place(window, int(regs.ebp), int(regs.edi));
                --*reinterpret_cast<int*>(viewport + 0x200);
            }
        });
    }

    pattern = hook::pattern("A1 ? ? ? ? 81 EC 80 00 00 00 85 C0 56 8B F1 75 39");
    if (!pattern.empty())
        shSetRes = safetyhook::create_inline(pattern.get_first(), SetRes);

    // SetRes, fullscreen: the refresh rate (+46C8h) is 0 (the adapter's default), the highest one up to the setting instead
    if (nFullScreenRefreshRateInHz > 0)
    {
        pattern = hook::pattern("89 9E C8 46 00 00 EB");
        if (!pattern.empty())
        {
            static auto RefreshRateHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
            {
                auto width = *reinterpret_cast<uint32_t*>(regs.esi + 0x46C0);
                auto height = *reinterpret_cast<uint32_t*>(regs.esi + 0x46C4);
                uint32_t rate = 0;
                DEVMODE mode = { .dmSize = sizeof(DEVMODE) };
                for (DWORD i = 0; EnumDisplaySettings(NULL, i, &mode); ++i)
                {
                    if (mode.dmPelsWidth == width && mode.dmPelsHeight == height && mode.dmDisplayFrequency > 1 &&
                        mode.dmDisplayFrequency <= uint32_t(nFullScreenRefreshRateInHz) && mode.dmDisplayFrequency > rate)
                        rate = mode.dmDisplayFrequency;
                }
                *reinterpret_cast<uint32_t*>(regs.esi + 0x46C8) = rate;
            });
        }
    }
}
