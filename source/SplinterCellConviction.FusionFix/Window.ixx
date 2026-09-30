module;

#include <stdafx.h>
#include <d3d9.h>

export module Window;

import ComVars;
import Splitscreen;

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
    return shsub_46E388.fastcall<int>(a1, edx, a2, a3, a4, 0, a6);
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

BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    SetWindowLong(hWnd, GWL_STYLE, GetWindowLong(hWnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);

    if (bEnableSplitscreen)
    {
        auto rect = GetSplitscreenRect();
        return SetWindowPos(hWnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    return SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
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
    }

    auto pattern = hook::pattern("A3 ? ? ? ? 83 BE");
    static auto GetPresentationParametersHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
    {
        auto PresentationParameters = (D3DPRESENT_PARAMETERS*)regs.edi;

        BackBufferWidth = PresentationParameters->BackBufferWidth;
        BackBufferHeight = PresentationParameters->BackBufferHeight;
    });
}