module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Windowed;

import ComVars;

bool bBorderlessWindowed = true;
bool bEnableWindowResize = false;
HWND GameHWND = NULL;

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
    if (!GameHWND)
        return;
    LONG lStyle = GetWindowLong(GameHWND, GWL_STYLE);
    lStyle &= ~(WS_HSCROLL | WS_VSCROLL);

    if (!bBorderlessWindowed)
    {
        lStyle |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
    }

    // STUB: window style code (borderless / resizable styles and applying them) authored by REDACTED removed

    auto [resW, resH] = GetRes();

    if (resW <= 0 || resH <= 0)
    {
        // STUB: removed code authored by REDACTED
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

    auto [resW, resH] = GetRes();

    const RECT monitor = GetMonitorBounds();
    const int desktopX = monitor.right - monitor.left;
    const int desktopY = monitor.bottom - monitor.top;

    int windowPosX = monitor.left + (desktopX - resW) / 2;
    int windowPosY = monitor.top + (desktopY - resH) / 2;

    GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, windowPosX, windowPosY, resW, resH, hWndParent, hMenu, hInstance, lpParam);
    ApplyWindowStyle();
    return GameHWND;
}

HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (hWndParent && hWndParent != GetDesktopWindow())
        return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    auto [resW, resH] = GetRes();

    const RECT monitor = GetMonitorBounds();
    const int desktopX = monitor.right - monitor.left;
    const int desktopY = monitor.bottom - monitor.top;

    int windowPosX = monitor.left + (desktopX - resW) / 2;
    int windowPosY = monitor.top + (desktopY - resH) / 2;

    GameHWND = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, windowPosX, windowPosY, resW, resH, hWndParent, hMenu, hInstance, lpParam);
    ApplyWindowStyle();
    return GameHWND;
}

LONG WINAPI SetWindowLongA_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
{
    if (bBorderlessWindowed)
    {
        ApplyWindowStyle();
        CenterWindowPosition();
        return dwNewLong;
    }
    else
    {
        dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
        // STUB: resizable window style code authored by REDACTED removed
    }

    return SetWindowLongA(hWnd, GWL_STYLE, dwNewLong);
}

LONG WINAPI SetWindowLongW_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
{
    if (bBorderlessWindowed)
    {
        ApplyWindowStyle();
        CenterWindowPosition();
        return dwNewLong;
    }
    else
    {
        dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
        // STUB: resizable window style code authored by REDACTED removed
    }

    return SetWindowLongW(hWnd, GWL_STYLE, dwNewLong);
}

BOOL WINAPI AdjustWindowRectEx_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
{
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

            auto bImproveWindowedMode = FixSettings().Int("MAIN.ImproveWindowedMode") != 0;

            {
                static bool bResChanged = false;

                WFP::onGameProcessEvent() += [bImproveWindowedMode]()
                {
                    if (!bImproveWindowedMode)
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

                if (bImproveWindowedMode)
                    IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
                        std::forward_as_tuple("CreateWindowExA", CreateWindowExA_Hook),
                        std::forward_as_tuple("CreateWindowExW", CreateWindowExW_Hook),
                        //std::forward_as_tuple("SetWindowLongA", SetWindowLongA_Hook),
                        //std::forward_as_tuple("SetWindowLongW", SetWindowLongW_Hook),
                        std::forward_as_tuple("AdjustWindowRectEx", AdjustWindowRectEx_Hook),
                        std::forward_as_tuple("SetWindowPos", SetWindowPos_Hook)
                );
            }
        };
    };
} Windowed;
