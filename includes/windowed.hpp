#pragma once
#include <windows.h>
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

    static std::tuple<int, int, int, int> beforeCreateWindow(int nWidth, int nHeight)
    {
        // fix the window to open at the center of the screen...
        HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info = { sizeof(MONITORINFOEX) };
        GetMonitorInfo(monitor, &info);
        DEVMODE devmode = {};
        devmode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
        DWORD DesktopX = devmode.dmPelsWidth;
        DWORD DesktopY = devmode.dmPelsHeight;

        int newWidth = nWidth;
        int newHeight = nHeight;

        if (bScaleWindow)
        {
            float fAspectRatio = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            newHeight = DesktopY;
            newWidth = static_cast<int>(newHeight * fAspectRatio);
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

        int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

        SetProcessDPIAware();

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
            AdjustWindowRectEx(&rect, lStyle, FALSE, exStyle);
            int outerW = rect.right - rect.left;
            int outerH = rect.bottom - rect.top;

            // Center by client area, not outer window
            HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX info = { sizeof(MONITORINFOEX) };
            GetMonitorInfo(monitor, &info);
            DEVMODE devmode = {};
            devmode.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);

            int posX = ((int)devmode.dmPelsWidth - desiredClientWidth) / 2 + rect.left;
            int posY = ((int)devmode.dmPelsHeight - desiredClientHeight) / 2 + rect.top;

            SetWindowPos(GameHWND, 0, posX, posY, outerW, outerH, SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        else
        {
            SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    static BOOL WINAPI AdjustWindowRect_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
    {
        if (!bBorderlessWindowed)
        {
            desiredClientWidth = lpRect->right - lpRect->left;
            desiredClientHeight = lpRect->bottom - lpRect->top;
            return AdjustWindowRect(lpRect, WS_CAPTION, bMenu);
        }
        else
        {
            afterCreateWindow();
            return AdjustWindowRect(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu);
        }
    }

    static BOOL WINAPI AdjustWindowRectEx_Hook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle)
    {
        if (!bBorderlessWindowed)
        {
            desiredClientWidth = lpRect->right - lpRect->left;
            desiredClientHeight = lpRect->bottom - lpRect->top;
            return AdjustWindowRectEx(lpRect, WS_CAPTION, bMenu, dwExStyle);
        }
        else
        {
            afterCreateWindow();
            return AdjustWindowRectEx(lpRect, GetWindowLong(GameHWND, GWL_STYLE), bMenu, dwExStyle);
        }
    }

    static BOOL WINAPI CenterWindowPosition(int nWidth, int nHeight)
    {
        // fix the window to open at the center of the screen...
        HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX info = { sizeof(MONITORINFOEX) };
        GetMonitorInfo(monitor, &info);
        DEVMODE devmode = {};
        devmode.dmSize = sizeof(DEVMODE);
        EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
        DWORD DesktopX = devmode.dmPelsWidth;
        DWORD DesktopY = devmode.dmPelsHeight;

        int newWidth = nWidth;
        int newHeight = nHeight;

        if (bScaleWindow)
        {
            float fAspectRatio = static_cast<float>(nWidth) / static_cast<float>(nHeight);
            newHeight = DesktopY;
            newWidth = static_cast<int>(newHeight * fAspectRatio);
        }

        if (bStretchWindow)
        {
            newHeight = DesktopY;
            newWidth = DesktopX;
        }

        int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
        int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

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
        if (bBorderlessWindowed)
        {
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return dwNewLong;
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
        if (bBorderlessWindowed)
        {
            afterCreateWindow();
            RECT rect;
            GetClientRect(hWnd, &rect);
            CenterWindowPosition(rect.right, rect.bottom);
            return dwNewLong;
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