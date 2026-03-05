module;

#include <stdafx.h>
#include <d3d9.h>

export module Window;

import ComVars;

SafetyHookInline shsub_46E388{};
int __fastcall sub_46E388(void* a1, void* edx, void* a2, int a3, int a4, int fullscreen, int a6)
{
    return shsub_46E388.fastcall<int>(a1, edx, a2, a3, a4, 0, a6);
}


enum WindowVerticalPos
{
    Center,
    Top,
    Bottom,
};

static BOOL WINAPI CenterWindowPosition(HWND hWnd, int nWidth, int nHeight, WindowVerticalPos put = Center)
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

    int WindowPosX = (int)(((float)DesktopX / 2.0f) - ((float)newWidth / 2.0f));
    int WindowPosY = (int)(((float)DesktopY / 2.0f) - ((float)newHeight / 2.0f));

    if (put == Top)
        return SetWindowPos(hWnd, 0, WindowPosX, 0, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
    else if (put == Bottom)
        return SetWindowPos(hWnd, 0, WindowPosX, DesktopY - newHeight, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);

    return SetWindowPos(hWnd, 0, WindowPosX, WindowPosY, newWidth, newHeight, SWP_NOZORDER | SWP_FRAMECHANGED);
}

bool bFocus = false;
SafetyHookInline shWndProc{};
int __fastcall WndProc(HDC _this, void* edx, UINT Msg, int wparam, unsigned int lparam)
{
    switch (Msg)
    {
    //case WM_SETFOCUS:
    //    SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    //    bFocus = true;
    //    break;
    //
    //case WM_KILLFOCUS:
    //    SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    //    bFocus = false;
    //    break;

    case WM_ACTIVATE:
        if (LOWORD(wparam) == WA_INACTIVE)
        {
            SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            bFocus = false;
        }
        else
        {
            SetWindowPos(*(HWND*)(*((uint32_t*)_this + 253) + 4), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            bFocus = true;
        }
        break;
    }

    auto ret = shWndProc.fastcall<int>(_this, edx, Msg, wparam, lparam);
    return ret;
}

BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);

    if (bEnableSplitscreen)
    {
        if (bInstance1)
            return CenterWindowPosition(hWnd, BackBufferWidth, BackBufferHeight, Top);
        else
            return CenterWindowPosition(hWnd, BackBufferWidth, BackBufferHeight, Bottom);
    }

    return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
}

SafetyHookInline shCreateSemaphoreA{};
HANDLE WINAPI CreateSemaphoreAHook(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName)
{
    auto ret = shCreateSemaphoreA.stdcall<HANDLE>(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        bInstance1 = false;
        return shCreateSemaphoreA.stdcall<HANDLE>(lpSemaphoreAttributes, lInitialCount, lMaximumCount, "Global\\sc5_semaphore2");
    }

    return ret;
}

export void InitWindow()
{
    if (bWindowedMode)
    {
        auto pattern = hook::pattern("55 8B EC 83 EC 40 53 56 83 C8 FF");
        shsub_46E388 = safetyhook::create_inline(pattern.get_first(), sub_46E388);

        pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 8B 45 0C");
        shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook)
        );

        pattern = hook::pattern("A3 ? ? ? ? 83 BE");
        static auto GetPresentationParametersHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            auto PresentationParameters = (D3DPRESENT_PARAMETERS*)regs.edi;

            BackBufferWidth = PresentationParameters->BackBufferWidth;
            BackBufferHeight = PresentationParameters->BackBufferHeight;
        });
    }

    if (bEnableSplitscreen)
    {
        shCreateSemaphoreA = safetyhook::create_inline(CreateSemaphoreA, CreateSemaphoreAHook);
    }
}