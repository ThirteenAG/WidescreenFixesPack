module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "include/WindowStyle.hpp"
#include "Shared/D3D9Options.hpp"


export module Window;

import ComVars;

// Native mode changes update the device, viewports and client together. Borderless uses the monitor dimensions.
export namespace DPLWindow
{
    inline int mode = 0, lastWindowMode = 1;
    inline LONG toggleDown = 0, toggleRequested = 0;
    inline bool changingMode = false, haveFramedRect = false;
    inline RECT framedRect{};
    inline int framedWidth = 0, framedHeight = 0;
    inline SafetyHookInline procedureHook;
    inline std::function<bool(int, int)> onModeChange;
    inline std::function<void()> onCreate;
    inline std::function<void(int, int)> publishDimensions;

    inline HWND Window() { return hWnd.is_initialized() ? hWnd.get() : nullptr; }

    inline bool Dimensions(int& width, int& height)
    {
        if (mode == 2)
        {
            const auto rect = WindowStyle::MonitorRect(Window());
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
        return width > 0 && height > 0 && width <= 16384 && height <= 16384;
    }

    inline bool ready = false;


    inline bool Place(int width, int height)
    {
        const auto window = Window();
        if (!ready || !window || mode == 0 || !Dimensions(width, height))
            return false;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::SquareCorners(window);
        WindowStyle::Place(window, width, height);
        if (mode == 1 && haveFramedRect && width == framedWidth && height == framedHeight)
            SetWindowPos(window, nullptr, framedRect.left, framedRect.top,
                framedRect.right - framedRect.left, framedRect.bottom - framedRect.top,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
        return true;
    }

    // Only the game's own device, not devices other plugins create for their own windows
    inline void Windowed(D3DPRESENT_PARAMETERS* present, HWND focus)
    {
        const auto window = Window();
        if (!window || (focus && focus != window && present->hDeviceWindow != window))
            return;
        if (mode == 0)
        {
            present->Windowed = FALSE;
            return;
        }
        int width = static_cast<int>(present->BackBufferWidth), height = static_cast<int>(present->BackBufferHeight);
        if (width <= 0 || height <= 0)
        {
            RECT client{};
            if (!GetClientRect(window, &client)) return;
            width = client.right; height = client.bottom;
        }
        if (!Dimensions(width, height)) return;
        present->BackBufferWidth = width;
        present->BackBufferHeight = height;
        present->Windowed = TRUE;
        present->FullScreen_RefreshRateInHz = 0;
        if (publishDimensions) publishDimensions(width, height);
        Place(int(present->BackBufferWidth), int(present->BackBufferHeight));
    }

    inline void FullscreenStyle()
    {
        if (mode != 0) return;
        const auto window = Window();
        const auto style = GetWindowLongW(window, GWL_STYLE);
        SetWindowLongW(window, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
        SetWindowPos(window, nullptr, 0, 0, 0, 0,
            SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::SquareCorners(window);
    }

    inline bool Frame(IDirect3DDevice9* device, bool blocked)
    {
        const auto window = Window();
        if (!ready || !window || GetForegroundWindow() != window)
        {
            InterlockedExchange(&toggleDown, 0);
            InterlockedExchange(&toggleRequested, 0);
            return false;
        }
        const bool down = (GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(VK_RETURN) & 0x8000);
        if (down)
        {
            if (!InterlockedCompareExchange(&toggleDown, 1, 0) && !blocked)
                InterlockedExchange(&toggleRequested, 1);
        }
        else InterlockedExchange(&toggleDown, 0);
        const bool suppressInput = down || toggleRequested;
        if (blocked)
        {
            InterlockedExchange(&toggleRequested, 0);
            return suppressInput;
        }
        if (!toggleRequested || changingMode || !device || device->TestCooperativeLevel() != D3D_OK || !onModeChange)
            return suppressInput;
        if (!InterlockedExchange(&toggleRequested, 0)) return suppressInput;
        const int previous = mode;
        RECT client{};
        GetClientRect(window, &client);
        if (previous == 1)
        {
            haveFramedRect = GetWindowRect(window, &framedRect) != FALSE;
            framedWidth = client.right;
            framedHeight = client.bottom;
        }
        changingMode = true;
        mode = previous ? 0 : lastWindowMode;
        FullscreenStyle();
        const int width = mode == 1 && haveFramedRect ? framedWidth : 0;
        const int height = mode == 1 && haveFramedRect ? framedHeight : 0;
        if (onModeChange(width, height))
        {
            if (previous) lastWindowMode = previous;
        }
        else
        {
            mode = previous;
            FullscreenStyle();
            if (onModeChange(client.right, client.bottom))
                OutputDebugStringA("DriverParallelLines: window mode change failed; previous mode restored.\n");
            else OutputDebugStringA("DriverParallelLines: previous window mode recovery failed.\n");
        }
        changingMode = false;
        return suppressInput;
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (ready && message == WM_NCCREATE && onCreate) onCreate();
        if (ready && window == Window() && wParam == VK_RETURN &&
            (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && ((lParam & (1u << 29)) || toggleDown))
        {
            if (message == WM_SYSKEYUP) InterlockedExchange(&toggleDown, 0);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&toggleDown, 1, 0)) InterlockedExchange(&toggleRequested, 1);
            return 0;
        }
        if (ready && mode != 0 && window == Window() && message == WM_CLOSE)
        {
            // The native message pump consumes WM_QUIT and unwinds the loop;
            // WinMain then destroys its own window and releases its services.
            PostQuitMessage(0);
            return 0;
        }
        return procedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline void Init()
    {
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        lastWindowMode = mode ? mode : 1;
        auto procedure = hook::pattern("55 8B EC 83 EC 20 8B 45 0C 48 48 56 57 0F 84 ? ? ? ? 83 E8 03 74 6F");
        if (!hWnd.is_initialized())
        {
            OutputDebugStringA("DriverParallelLines: native window procedure unavailable.\n");
            return;
        }
        procedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);
        D3D9Options::presentParameters = Windowed;
        ready = procedureHook && D3D9Options::HookCreation();
        if (!ready)
        {
            D3D9Options::presentParameters = nullptr;
            OutputDebugStringA("DriverParallelLines: window mode unavailable; fullscreen retained.\n");
        }
    }
}
