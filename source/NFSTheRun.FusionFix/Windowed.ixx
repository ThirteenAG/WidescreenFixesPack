module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Windowed;

import ComVars;

// MAIN.WindowMode: 0 the game's own window, 1 a framed window that can be resized, 2 borderless
bool bOwnWindow = false;
bool bBorderlessWindowed = true;
bool bEnableWindowResize = false;
HWND GameHWND = NULL;

namespace
{
    RECT NativeBounds{};
    LONG NativeStyle = 0;
    bool HaveNativeBounds = false;

    void RememberNativeWindow()
    {
        HaveNativeBounds = GameHWND && GetWindowRect(GameHWND, &NativeBounds);
        if (HaveNativeBounds)
            NativeStyle = GetWindowLongW(GameHWND, GWL_STYLE);
    }

    void RestoreNativeWindow()
    {
        if (!GameHWND || !HaveNativeBounds)
            return;

        constexpr DWORD transient = WS_VISIBLE | WS_MINIMIZE | WS_MAXIMIZE;
        const LONG current = GetWindowLongW(GameHWND, GWL_STYLE);
        SetWindowLongW(GameHWND, GWL_STYLE, (NativeStyle & ~transient) | (current & transient));
        const int width = NativeBounds.right - NativeBounds.left;
        const int height = NativeBounds.bottom - NativeBounds.top;
        WindowStyle::detail::AllowSize(GameHWND, width, height);
        SetWindowPos(GameHWND, nullptr, NativeBounds.left, NativeBounds.top, width, height,
            SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

void ReadWindowMode()
{
    const int mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
    bOwnWindow = mode == 0;
    bBorderlessWindowed = mode == 2;
    bEnableWindowResize = mode == 1;
}

namespace
{
    struct WindowResolution
    {
        uint32_t width;
        uint32_t height;
        uint32_t hertz;
    };

    struct VideoSelection
    {
        uint32_t mode;
        uint32_t screen;
        WindowResolution resolution;
    };

    std::map<uintptr_t, VideoSelection> WindowSelections;
    WindowResolution AppliedWindowResolution{};

    void RememberWindowResolution(SafetyHookContext& regs)
    {
        WindowSelections.erase(regs.esi);
        const bool fullscreen = *reinterpret_cast<bool*>(regs.esi + 8);
        if (fullscreen)
            return;

        const auto hertz = *reinterpret_cast<uint32_t*>(regs.esp + 8);
        WindowResolution resolution{ regs.ebx, regs.ebp, hertz };
        RECT client{};
        HWND window = hWnd.get();
        if (window && GetClientRect(window, &client) && client.right >= 640 && client.bottom >= 480 && !IsIconic(window))
        {
            resolution.width = client.right;
            resolution.height = client.bottom;
        }

        if (resolution.width < 640 || resolution.height < 480 || resolution.width > 16384 || resolution.height > 16384)
            return;

        const auto screen = *reinterpret_cast<uint32_t*>(regs.esi + 4);
        WindowSelections.insert_or_assign(regs.esi, VideoSelection{ regs.eax, screen, resolution });
    }

    void ApplyWindowResolution(SafetyHookContext& regs)
    {
        auto selection = WindowSelections.find(regs.edi);
        if (selection == WindowSelections.end())
            return;

        const auto mode = *reinterpret_cast<uint32_t*>(regs.edi);
        const auto screen = *reinterpret_cast<uint32_t*>(regs.edi + 4);
        const bool fullscreen = *reinterpret_cast<bool*>(regs.edi + 8);
        if (fullscreen || mode != selection->second.mode || screen != selection->second.screen)
        {
            WindowSelections.erase(selection);
            return;
        }

        // Keep an unlisted window size when the options handler reapplies its unchanged mode index.
        AppliedWindowResolution = selection->second.resolution;
        regs.ebx = reinterpret_cast<uintptr_t>(&AppliedWindowResolution);
    }
}

RECT GetMonitorBounds()
{
    const HWND window = GameHWND && IsWindow(GameHWND) ? GameHWND : GetDesktopWindow();
    const HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info{ sizeof(info) };
    if (GetMonitorInfo(monitor, &info))
        return info.rcMonitor;
    return { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
}

void ApplyWindowStyle()
{
    if (!GameHWND || bOwnWindow)
        return;
    LONG lStyle = GetWindowLong(GameHWND, GWL_STYLE);
    lStyle &= ~(WS_HSCROLL | WS_VSCROLL);

    if (bBorderlessWindowed)
        lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    else
    {
        lStyle |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
        if (bEnableWindowResize)
            lStyle |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
    }

    SetWindowLong(GameHWND, GWL_STYLE, lStyle);

    auto [resW, resH] = GetRes();

    if (resW <= 0 || resH <= 0)
    {
        SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        return;
    }

    const RECT monitor = GetMonitorBounds();
    const int desktopX = monitor.right - monitor.left;
    const int desktopY = monitor.bottom - monitor.top;

    if (bBorderlessWindowed)
    {
        // No frame — window size == client size, just center it
        int posX = monitor.left + (desktopX - resW) / 2;
        int posY = monitor.top + (desktopY - resH) / 2;
        SetWindowPos(GameHWND, 0, posX, posY, resW, resH, SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    else
    {
        // Account for caption/borders so client rect == GetRes()
        RECT rect = { 0, 0, resW, resH };
        LONG exStyle = GetWindowLong(GameHWND, GWL_EXSTYLE);
        AdjustWindowRectEx(&rect, lStyle, FALSE, exStyle);
        int outerW = rect.right - rect.left;
        int outerH = rect.bottom - rect.top;

        int posX = monitor.left + (desktopX - resW) / 2 + rect.left;
        int posY = monitor.top + (desktopY - resH) / 2 + rect.top;
        WindowStyle::detail::AllowSize(GameHWND, outerW, outerH);
        SetWindowPos(GameHWND, 0, posX, posY, outerW, outerH, SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

BOOL WINAPI CenterWindowPosition()
{
    auto [resW, resH] = GetRes();

    const RECT monitor = GetMonitorBounds();
    const int desktopX = monitor.right - monitor.left;
    const int desktopY = monitor.bottom - monitor.top;

    int windowPosX = monitor.left + (desktopX - resW) / 2;
    int windowPosY = monitor.top + (desktopY - resH) / 2;

    return SetWindowPos(GameHWND, 0, windowPosX, windowPosY, resW, resH, SWP_NOZORDER | SWP_FRAMECHANGED);
}

void UpdateSystemCursor()
{
    static int lastVisibility = -1;
    static int lastObserved = -1;
    const HWND window = hWnd.get();
    if (!window || window != GetForegroundWindow())
    {
        lastVisibility = -1;
        lastObserved = -1;
        return;
    }

    POINT cursor{};
    RECT client{};
    CURSORINFO info{ sizeof(info) };
    if (!GetCursorPos(&cursor) || !GetClientRect(window, &client) || !GetCursorInfo(&info))
        return;
    MapWindowPoints(window, nullptr, reinterpret_cast<POINT*>(&client), 2);
    const bool shouldShow = !PtInRect(&client, cursor);
    const bool visible = (info.flags & CURSOR_SHOWING) != 0;
    if (lastVisibility == int(shouldShow) && lastObserved == int(visible))
        return;
    lastVisibility = int(shouldShow);
    lastObserved = int(visible);
    if (visible == shouldShow)
        return;

    // The counter belongs to the window thread and may have been changed by the game.
    for (int i = 0; i < 32; ++i)
    {
        const int count = ShowCursor(shouldShow);
        if (shouldShow ? count >= 0 : count < 0)
            return;
    }
    lastVisibility = -1;
}

HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (hWndParent && hWndParent != GetDesktopWindow())
        return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    RememberNativeWindow();
    ApplyWindowStyle();
    return GameHWND;
}

HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (hWndParent && hWndParent != GetDesktopWindow())
        return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    GameHWND = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    RememberNativeWindow();
    ApplyWindowStyle();
    return GameHWND;
}

BOOL WINAPI AdjustWindowRectEx_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
    if (bOwnWindow)
        return AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
    if (bBorderlessWindowed)
    {
        ApplyWindowStyle();
        return AdjustWindowRectEx(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu, dwExStyle);
    }

    return AdjustWindowRectEx(lpRect, WS_CAPTION, bMenu, dwExStyle);
}

BOOL WINAPI SetWindowPos_Hook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    BOOL res = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    if (res && GameHWND && hWnd == GameHWND && bBorderlessWindowed)
    {
        ApplyWindowStyle();
        CenterWindowPosition();
    }
    return res;
}

class Windowed
{
public:
    Windowed()
    {
        // After the game's code is unpacked: a protected exe (Origin) isn't when the plugin loads
        WFP::onInitEvent() += []()
        {
            // Per monitor DPI awareness, so the game gets real pixels on scaled desktops
            WindowedModeWrapper::EnableDpiAwareness();

            // No rounded corners on Windows 11 (DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_DONOTROUND) in every window
            // mode, so the game image keeps square corners; nothing happens before Windows 11
            WFP::onGameProcessEvent() += []()
            {
                static HWND cornersSet = NULL;
                HWND window = hWnd.get();
                if (!window || window == cornersSet)
                    return;
                cornersSet = window;

                using SetAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
                static auto setAttribute = []() -> SetAttributeFn
                {
                    auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                    return dwm ? reinterpret_cast<SetAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
                }();
                int preference = 1;
                if (setAttribute)
                    setAttribute(window, 33, &preference, sizeof(preference));
            };

            ReadWindowMode();

            {
                static bool bResChanged = false;
                static std::atomic<bool> bModeChanged = false;
                FixSettings().OnChange("MAIN.WindowMode", []() { bModeChanged = true; });

                WFP::onGameProcessEvent() += []()
                {
                    if (bModeChanged.exchange(false))
                    {
                        const bool wasOwnWindow = bOwnWindow;
                        if (wasOwnWindow)
                            RememberNativeWindow();
                        ReadWindowMode();
                        if (bOwnWindow && !wasOwnWindow)
                            RestoreNativeWindow();
                        else if (!bOwnWindow)
                            ApplyWindowStyle();
                    }

                    if (bOwnWindow)
                        return;

                    UpdateSystemCursor();

                    if (bResChanged)
                    {
                        static std::pair<int, int> cachedRes = { 0, 0 };
                        auto currentRes = GetRes();
                        if (currentRes != cachedRes)
                        {
                            cachedRes = currentRes;
                            ApplyWindowStyle();
                            bResChanged = false;
                        }
                    }
                };

                //fb::OptionsVideoHandler::reset
                auto pattern = hook::pattern("E8 ? ? ? ? 8B 4E ? 8A 56 ? 83 C4");
                static auto resetHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs)
                {
                    RememberWindowResolution(regs);
                    bResChanged = true;
                });

                pattern = hook::pattern("8D 1C 81 8B 0B 51 83 C6 ? 68");
                static auto windowResolutionHook = safetyhook::create_mid(pattern.get_first(3), [](SafetyHookContext& regs)
                {
                    ApplyWindowResolution(regs);
                    bResChanged = true;
                });

                IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                        std::forward_as_tuple("CreateWindowExA", CreateWindowExA_Hook),
                        std::forward_as_tuple("CreateWindowExW", CreateWindowExW_Hook),
                        std::forward_as_tuple("AdjustWindowRectEx", AdjustWindowRectEx_Hook),
                        std::forward_as_tuple("SetWindowPos", SetWindowPos_Hook)
                );
            }
        };
    };
} Windowed;
