#pragma once
#include <windows.h>
#include <algorithm>
#include <tuple>
#include <cstdint>

namespace WindowedModeWrapper
{
    static bool bBorderlessWindowed = true;
    static bool bEnableWindowResize = false;
    static bool bScaleWindow = false;
    static bool bStretchWindow = false;
    static HWND GameHWND = NULL;
    static int desiredClientWidth = 0;
    static int desiredClientHeight = 0;
    static bool s_cursorOnNcArea = false;

    static void EnableDpiAwareness()
    {
        using SetProcessContext = BOOL(WINAPI*)(HANDLE);
        using SetThreadContext = HANDLE(WINAPI*)(HANDLE);
        auto user32 = GetModuleHandleW(L"user32.dll");
        auto process = reinterpret_cast<SetProcessContext>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (!process || !process(reinterpret_cast<HANDLE>(-4)))
        {
            auto thread = reinterpret_cast<SetThreadContext>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
            if (thread)
                thread(reinterpret_cast<HANDLE>(-4));
            else
                SetProcessDPIAware();
        }
    }

    static BOOL AdjustFrame(LPRECT rect, DWORD style, BOOL menu, DWORD exStyle)
    {
        using GetWindowDpi = UINT(WINAPI*)(HWND);
        using GetSystemDpi = UINT(WINAPI*)();
        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        auto user32 = GetModuleHandleW(L"user32.dll");
        auto windowDpi = reinterpret_cast<GetWindowDpi>(GetProcAddress(user32, "GetDpiForWindow"));
        auto systemDpi = reinterpret_cast<GetSystemDpi>(GetProcAddress(user32, "GetDpiForSystem"));
        auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        UINT dpi = windowDpi && GameHWND ? windowDpi(GameHWND) : (systemDpi ? systemDpi() : 0);
        return adjust && dpi ? adjust(rect, style, menu, exStyle, dpi) : AdjustWindowRectEx(rect, style, menu, exStyle);
    }

    static MONITORINFOEX WindowMonitor()
    {
        MONITORINFOEX info{ sizeof(info) };
        const auto window = GameHWND && IsWindow(GameHWND) ? GameHWND : nullptr;
        auto monitor = window ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST)
            : MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        if (!GetMonitorInfo(monitor, &info))
        {
            monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
            if (!GetMonitorInfo(monitor, &info))
            {
                info.rcMonitor = { 0, 0, std::max(GetSystemMetrics(SM_CXSCREEN), 640), std::max(GetSystemMetrics(SM_CYSCREEN), 480) };
                info.rcWork = info.rcMonitor;
            }
        }
        return info;
    }

    static std::tuple<int, int, int, int> beforeCreateWindow(int nWidth, int nHeight)
    {
        EnableDpiAwareness();
        // fix the window to open at the center of the screen...
        const auto info = WindowMonitor();
        const auto DesktopX = info.rcMonitor.right - info.rcMonitor.left;
        const auto DesktopY = info.rcMonitor.bottom - info.rcMonitor.top;

        int newWidth = nWidth;
        int newHeight = nHeight;

        if (bScaleWindow)
        {
            // As large as the screen allows at the game's aspect ratio: by height, or by width when it's wider
            float fAspectRatio = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            newHeight = DesktopY;
            newWidth = static_cast<int>(newHeight * fAspectRatio);
            if (newWidth > static_cast<int>(DesktopX))
            {
                newWidth = DesktopX;
                newHeight = static_cast<int>(newWidth / fAspectRatio);
            }
        }

        if (bStretchWindow)
        {
            newHeight = DesktopY;
            newWidth = DesktopX;
        }

        if (bBorderlessWindowed || desiredClientWidth <= 0 || desiredClientHeight <= 0)
        {
            desiredClientWidth = newWidth;
            desiredClientHeight = newHeight;
        }

        int WindowPosX = info.rcMonitor.left + (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = info.rcMonitor.top + (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));


        return std::make_tuple(WindowPosX, WindowPosY, newWidth, newHeight);
    }

    static void afterCreateWindow()
    {
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

        if (!bBorderlessWindowed && desiredClientWidth > 0 && desiredClientHeight > 0)
        {
            // Compute outer window size so client rect = desired dimensions
            RECT rect = { 0, 0, desiredClientWidth, desiredClientHeight };
            LONG exStyle = GetWindowLong(GameHWND, GWL_EXSTYLE);
            AdjustFrame(&rect, lStyle, FALSE, exStyle);
            int outerW = rect.right - rect.left;
            int outerH = rect.bottom - rect.top;

            // Center by client area, not outer window
            const auto info = WindowMonitor();
            const auto desktopWidth = info.rcMonitor.right - info.rcMonitor.left;
            const auto desktopHeight = info.rcMonitor.bottom - info.rcMonitor.top;

            int posX = info.rcMonitor.left + (desktopWidth - desiredClientWidth) / 2 + rect.left;
            int posY = info.rcMonitor.top + (desktopHeight - desiredClientHeight) / 2 + rect.top;

            UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED;
            // The frame may extend beyond the monitor; keep the requested client size.
            if (desiredClientWidth >= desktopWidth || desiredClientHeight >= desktopHeight)
                flags |= SWP_NOSENDCHANGING;
            SetWindowPos(GameHWND, 0, posX, posY, outerW, outerH, flags);
        }
        else
        {
            SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }

        // No rounded corners on Windows 11 (DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_DONOTROUND), so the game image
        // keeps square corners; nothing happens before Windows 11
        using SetAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttributeFn
        {
            auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return dwm ? reinterpret_cast<SetAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        int preference = 1;
        if (setAttribute && GameHWND)
            setAttribute(GameHWND, 33, &preference, sizeof(preference));
    }

    static BOOL WINAPI AdjustWindowRect_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
    {
        if (!bBorderlessWindowed)
        {
            desiredClientWidth = lpRect->right - lpRect->left;
            desiredClientHeight = lpRect->bottom - lpRect->top;
            return AdjustFrame(lpRect, WS_CAPTION, bMenu, 0);
        }
        else
        {
            afterCreateWindow();
            return AdjustFrame(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu, 0);
        }
    }

    static BOOL WINAPI AdjustWindowRectEx_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
    {
        if (!bBorderlessWindowed)
        {
            desiredClientWidth = lpRect->right - lpRect->left;
            desiredClientHeight = lpRect->bottom - lpRect->top;
            return AdjustFrame(lpRect, WS_CAPTION, bMenu, dwExStyle);
        }
        else
        {
            afterCreateWindow();
            return AdjustFrame(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu, dwExStyle);
        }
    }

    static BOOL WINAPI CenterWindowPosition(int nWidth, int nHeight)
    {
        // fix the window to open at the center of the screen...
        const auto info = WindowMonitor();
        const auto DesktopX = info.rcMonitor.right - info.rcMonitor.left;
        const auto DesktopY = info.rcMonitor.bottom - info.rcMonitor.top;

        int newWidth = nWidth;
        int newHeight = nHeight;

        if (bScaleWindow)
        {
            // As large as the screen allows at the game's aspect ratio: by height, or by width when it's wider
            float fAspectRatio = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            newHeight = DesktopY;
            newWidth = static_cast<int>(newHeight * fAspectRatio);
            if (newWidth > static_cast<int>(DesktopX))
            {
                newWidth = DesktopX;
                newHeight = static_cast<int>(newWidth / fAspectRatio);
            }
        }

        if (bStretchWindow)
        {
            newHeight = DesktopY;
            newWidth = DesktopX;
        }

        int WindowPosX = info.rcMonitor.left + (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = info.rcMonitor.top + (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

        return SetWindowPos(GameHWND, 0, WindowPosX, WindowPosY, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (hWndParent && hWndParent != GetDesktopWindow())
            return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

        auto [WindowPosX, WindowPosY, newWidth, newHeight] = beforeCreateWindow(nWidth, nHeight);
        GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, WindowPosX, WindowPosY, newWidth, newHeight, hWndParent, hMenu, hInstance, lpParam);
        afterCreateWindow();
        return GameHWND;
    }

    static HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        if (hWndParent && hWndParent != GetDesktopWindow())
            return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

        auto [WindowPosX, WindowPosY, newWidth, newHeight] = beforeCreateWindow(nWidth, nHeight);
        GameHWND = CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, WindowPosX, WindowPosY, newWidth, newHeight, hWndParent, hMenu, hInstance, lpParam);
        afterCreateWindow();
        return GameHWND;
    }

    static LONG WINAPI SetWindowLongA_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (hWnd != GameHWND || nIndex != GWL_STYLE)
            return SetWindowLongA(hWnd, nIndex, dwNewLong);
        if (bBorderlessWindowed)
        {
            const LONG previous = GetWindowLong(hWnd, GWL_STYLE);
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return previous;
        }
        else
        {
            dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
            if (bEnableWindowResize)
                dwNewLong |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        return SetWindowLongA(hWnd, GWL_STYLE, dwNewLong);
    }

    static LONG WINAPI SetWindowLongW_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        if (hWnd != GameHWND || nIndex != GWL_STYLE)
            return SetWindowLongW(hWnd, nIndex, dwNewLong);
        if (bBorderlessWindowed)
        {
            const LONG previous = GetWindowLong(hWnd, GWL_STYLE);
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return previous;
        }
        else
        {
            dwNewLong |= (WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU);
            if (bEnableWindowResize)
                dwNewLong |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
        }

        return SetWindowLongW(hWnd, GWL_STYLE, dwNewLong);
    }

    static BOOL WINAPI SetWindowPos_Hook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
    {
        BOOL res = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
        if (bBorderlessWindowed)
        {
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return TRUE;
        }
        return res;
    }

    static BOOL WINAPI MoveWindow_Hook(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
    {
        BOOL res = MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
        if (bBorderlessWindowed && GameHWND == hWnd)
        {
            CenterWindowPosition(nWidth, nHeight);
        }
        return res;
    }

    static int WINAPI ShowCursor_Hook(BOOL bShow)
    {
        if (!bBorderlessWindowed && GameHWND && !bShow)
        {
            POINT pt;
            GetCursorPos(&pt);
            RECT clientRect;
            GetClientRect(GameHWND, &clientRect);
            MapWindowPoints(GameHWND, NULL, (LPPOINT)&clientRect, 2);

            if (!PtInRect(&clientRect, pt))
            {
                if (!s_cursorOnNcArea)
                {
                    // Entering NC area — force counter to 0 (visible)
                    s_cursorOnNcArea = true;
                    int count;
                    do { count = ShowCursor(TRUE); } while (count < 0);
                }
                return 0; // Suppress the hide
            }
            else if (s_cursorOnNcArea)
            {
                // Back on client area — let game hide normally
                s_cursorOnNcArea = false;
            }
        }

        return ShowCursor(bShow);
    }
};
