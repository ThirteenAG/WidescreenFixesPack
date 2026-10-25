module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/SplinterCell/Window.hpp"
#include <d3d9.h>

export module Window;

import ComVars;
import Splitscreen;

bool DeviceWindowed = false;
HWND GameWindow = nullptr;
SafetyHookInline shsub_46E388{};
// UD3DRenderDevice::SetRes(viewport, width, height, fullscreen, ...)
int __fastcall sub_46E388(void* a1, void* edx, void* a2, int a3, int a4, int fullscreen, int a6)
{
    if (bEnableSplitscreen)
    {
        auto rect = GetSplitscreenRect();
        a3 = rect.right - rect.left;
        a4 = rect.bottom - rect.top;
    }
    static bool first = true;
    if (std::exchange(first, false) || bEnableSplitscreen)
        fullscreen = 0;
    DeviceWindowed = fullscreen == 0;
    if (DeviceWindowed && !bEnableSplitscreen && FixSettings().Int("MAIN.WindowMode") == 2)
    {
        auto area = SCWindow::Monitor(GameWindow).rcMonitor;
        a3 = area.right - area.left;
        a4 = area.bottom - area.top;
    }
    return shsub_46E388.fastcall<int>(a1, edx, a2, a3, a4, fullscreen, a6);
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
        if (!DeviceWindowed)
            break;
        // split screen: both windows stay over the taskbar
        if (LOWORD(wparam) == WA_INACTIVE && !bEnableSplitscreen)
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

// UWindowsViewport::SetFocus(focus) (sub_8F17D8), sets +414h, captures the mouse only in fullscreen (virtual +8Ch),
// windowed it waits for a key press or a click (the cursor doesn't show): capture it (virtual +B0h, captured at +40Ch) when it gets focus
SafetyHookInline shViewportSetFocus{};
int __fastcall ViewportSetFocus(uintptr_t viewport, void* edx, int32_t focus)
{
    auto result = shViewportSetFocus.fastcall<int>(viewport, edx, focus);
    auto vtable = *reinterpret_cast<uintptr_t**>(viewport);
    auto hWnd = *reinterpret_cast<HWND*>(*reinterpret_cast<uintptr_t*>(viewport + 0x3F4) + 4);
    if (focus && *reinterpret_cast<int32_t*>(viewport + 0x40C) == 0 && GetForegroundWindow() == hWnd &&
        reinterpret_cast<int(__thiscall*)(uintptr_t)>(vtable[0x8C / 4])(viewport) == 0)
        reinterpret_cast<void(__thiscall*)(uintptr_t, int32_t, int32_t, int32_t)>(vtable[0xB0 / 4])(viewport, 1, 1, 0);
    return result;
}

BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    GameWindow = hWnd;
    SCWindow::SquareCorners(hWnd);
    const int width = BackBufferWidth > 0 ? BackBufferWidth : cx;
    const int height = BackBufferHeight > 0 ? BackBufferHeight : cy;
    if (!bEnableSplitscreen)
        SCWindow::Track(hWnd, FixSettings().Int("MAIN.WindowMode"), DeviceWindowed, width, height);
    if (!DeviceWindowed)
        return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
    if (bEnableSplitscreen)
    {
        SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);
        auto rect = GetSplitscreenRect();
        return SetWindowPos(hWnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    SCWindow::Place(hWnd, width, height, FixSettings().Int("MAIN.WindowMode") == 2);
    const auto result = TRUE;

    // the window starts without focus
    static bool focused = false;
    if (!std::exchange(focused, true))
        SetForegroundWindow(hWnd);
    return result;
}

export void InitWindow()
{
    if (bWindowedMode)
    {
        auto pattern = hook::pattern("55 8B EC 83 EC 40 53 56 83 C8 FF");
        shsub_46E388 = safetyhook::create_inline(pattern.get_first(), sub_46E388);

        pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC ? ? ? ? A1 ? ? ? ? 33 C4 89 84 24 ? ? ? ? 8B 45 0C");
        shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

        pattern = hook::pattern("53 56 33 C0 33 DB 57 8B 7C 24 10 3B FB 0F 95 C0 8B F1 33 C9 39 9E 14 04 00 00");
        shViewportSetFocus = safetyhook::create_inline(pattern.get_first(), ViewportSetFocus);

        IATHook::Replace(GetModuleHandleA(NULL), "USER32.DLL",
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook)
        );
    }

    auto pattern = hook::pattern("A3 ? ? ? ? 83 BE");
    static auto GetPresentationParametersHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto PresentationParameters = (D3DPRESENT_PARAMETERS*)regs.edi;

        BackBufferWidth = PresentationParameters->BackBufferWidth;
        BackBufferHeight = PresentationParameters->BackBufferHeight;
        SCWindow::SquareCorners(PresentationParameters->hDeviceWindow);
    });
}