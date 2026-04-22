module;

#include <stdafx.h>

export module Windowed;

import ComVars;

bool bBorderlessWindowed = true;
bool bEnableWindowResize = false;
HWND GameHWND = NULL;

void GetDesktopDimensions(DWORD& desktopX, DWORD& desktopY)
{
    HMONITOR monitor = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(monitor, &info);
    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);
    desktopX = devmode.dmPelsWidth;
    desktopY = devmode.dmPelsHeight;
}

void ApplyWindowStyle()
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

    auto [resW, resH] = GetRes();

    if (resW <= 0 || resH <= 0)
    {
        SetWindowPos(GameHWND, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
        return;
    }

    DWORD desktopX, desktopY;
    GetDesktopDimensions(desktopX, desktopY);

    if (bBorderlessWindowed)
    {
        // No frame — window size == client size, just center it
        int posX = ((int)desktopX - resW) / 2;
        int posY = ((int)desktopY - resH) / 2;
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

        int posX = ((int)desktopX - resW) / 2 + rect.left;
        int posY = ((int)desktopY - resH) / 2 + rect.top;
        SetWindowPos(GameHWND, 0, posX, posY, outerW, outerH, SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

BOOL WINAPI CenterWindowPosition()
{
    auto [resW, resH] = GetRes();

    DWORD desktopX, desktopY;
    GetDesktopDimensions(desktopX, desktopY);

    int windowPosX = ((int)desktopX - resW) / 2;
    int windowPosY = ((int)desktopY - resH) / 2;

    return SetWindowPos(GameHWND, 0, windowPosX, windowPosY, resW, resH, SWP_NOZORDER | SWP_FRAMECHANGED);
}

int WINAPI DisplayCursor(BOOL bShow)
{
    static bool s_cursorOnNcArea = false;

    if (hWnd != GetForegroundWindow())
        return 0;

    if (hWnd && !bShow)
    {
        POINT pt;
        GetCursorPos(&pt);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        MapWindowPoints(hWnd, NULL, (LPPOINT)&clientRect, 2);

        if (!PtInRect(&clientRect, pt))
        {
            if (!s_cursorOnNcArea)
            {
                s_cursorOnNcArea = true;
                int count;
                do { count = ShowCursor(TRUE); } while (count < 0);
            }
            return 0;
        }
        else if (s_cursorOnNcArea)
        {
            s_cursorOnNcArea = false;
        }
    }

    return ShowCursor(bShow);
}

HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (hWndParent && hWndParent != GetDesktopWindow())
        return CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    SetProcessDPIAware();

    auto [resW, resH] = GetRes();

    DWORD desktopX, desktopY;
    GetDesktopDimensions(desktopX, desktopY);

    int windowPosX = ((int)desktopX - resW) / 2;
    int windowPosY = ((int)desktopY - resH) / 2;

    GameHWND = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, windowPosX, windowPosY, resW, resH, hWndParent, hMenu, hInstance, lpParam);
    ApplyWindowStyle();
    return GameHWND;
}

HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle,
    int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    if (hWndParent && hWndParent != GetDesktopWindow())
        return CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

    SetProcessDPIAware();

    auto [resW, resH] = GetRes();

    DWORD desktopX, desktopY;
    GetDesktopDimensions(desktopX, desktopY);

    int windowPosX = ((int)desktopX - resW) / 2;
    int windowPosY = ((int)desktopY - resH) / 2;

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
        if (bEnableWindowResize)
            dwNewLong |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
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
        if (bEnableWindowResize)
            dwNewLong |= (WS_MAXIMIZEBOX | WS_THICKFRAME);
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
    if (bBorderlessWindowed)
    {
        ApplyWindowStyle();
        CenterWindowPosition();
        return TRUE;
    }
    return res;
}

class Windowed
{
public:
    Windowed()
    {
        CIniReader iniReader("");
        auto bImproveWindowedMode = iniReader.ReadInteger("MAIN", "ImproveWindowedMode", 1) != 0;

        if (bImproveWindowedMode)
        {
            static bool bResChanged = false;

            WFP::onGameProcessEvent() += []()
            {
                DisplayCursor(FALSE);

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
            static auto resetHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bResChanged = true;
            });

            //fb::OptionsVideoHandler::applyVideoSettings
            pattern = hook::pattern("51 83 C6 ? 68");
            static auto applyVideoSettingsHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                bResChanged = true;
            });

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
} Windowed;