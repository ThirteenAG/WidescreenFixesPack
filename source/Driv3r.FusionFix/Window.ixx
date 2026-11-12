module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "include/WindowStyle.hpp"


export module Window;

import ComVars;

export namespace Driv3rWindow
{
    inline SafetyHookMid modeHook, publicationHook;
    inline void (*onPrepared)(void*) = nullptr;
    inline std::function<bool(int, int)> onModeChange;
    inline LONG toggleDown = 0, toggleRequested = 0;
    inline bool changingMode = false, haveFramedRect = false;
    inline RECT framedRect{};
    inline int framedWidth = 0, framedHeight = 0, lastWindowMode = 1;
    inline SafetyHookInline procedureHook;
    inline SafetyHookMid shutdownHook;
    inline GameRef<uint32_t> PresentedFrames;
    inline int mode = 0, requestedIndex = -1;

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

    inline bool Place(int width, int height)
    {
        const auto window = Window();
        if (!window)
            return false;
        WindowStyle::SquareCorners(window);
        if (mode == 0 || !Dimensions(width, height))
            return false;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::Place(window, width, height);
        if (mode == 1 && haveFramedRect && width == framedWidth && height == framedHeight)
            SetWindowPos(window, nullptr, framedRect.left, framedRect.top,
                framedRect.right - framedRect.left, framedRect.bottom - framedRect.top,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
        return true;
    }

    inline void Request(SafetyHookContext& context)
    {
        if (mode == 0 || reinterpret_cast<uint8_t*>(context.eax) != Renderer.get_ptr())
            return;
        auto index = *reinterpret_cast<const int**>(context.esp + 8);
        if (index) requestedIndex = *index;
        auto requested = *reinterpret_cast<uint8_t**>(context.esp + 12);
        if (requested)
            *requested = 1;
    }

    inline bool Prepare(void* context, int width, int height)
    {
        if (mode == 0 || context != Renderer.get_ptr() || !Dimensions(width, height))
            return false;
        auto& present = *reinterpret_cast<D3DPRESENT_PARAMETERS*>(Renderer.get_ptr() + 0x160);
        // The native windowed branch uses zero dimensions, which mean the
        // client size to D3D9 but cannot be published as the game's resolution.
        present.BackBufferWidth = UINT(width);
        present.BackBufferHeight = UINT(height);
        present.Windowed = TRUE;
        present.FullScreen_RefreshRateInHz = 0;
        *reinterpret_cast<uint8_t*>(Renderer.get_ptr() + 0x14C) = 1;
        Place(width, height);
        return true;
    }

    inline void Configured(SafetyHookContext& context)
    {
        auto native = reinterpret_cast<void*>(context.ebx);
        if (Prepare(native, *reinterpret_cast<int*>(context.esp + 8), *reinterpret_cast<int*>(context.esp + 12)) && onPrepared)
            onPrepared(native);
    }

    inline bool SynchronizeViewport(void* view, int requestedIndex, int actualIndex, int width, int height, bool built)
    {
        auto bytes = static_cast<uint8_t*>(view);
        auto& currentIndex = *reinterpret_cast<int32_t*>(bytes + 0x1098);
        if (currentIndex == requestedIndex && currentIndex != actualIndex)
        {
            currentIndex = actualIndex;
            *reinterpret_cast<int32_t*>(bytes + 0x105C) = width;
            *reinterpret_cast<int32_t*>(bytes + 0x1060) = height;
        }
        if (!built || width <= 0 || height <= 0)
            return false;
        *reinterpret_cast<int32_t*>(bytes + 0x105C) = width;
        *reinterpret_cast<int32_t*>(bytes + 0x1060) = height;
        return true;
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
        if (!window || GetForegroundWindow() != window)
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
                OutputDebugStringA("Driv3r: window mode change failed; previous mode restored.\n");
            else OutputDebugStringA("Driv3r: previous window mode recovery failed.\n");
        }
        changingMode = false;
        return suppressInput;
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (window == Window() && wParam == VK_RETURN &&
            (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && ((lParam & (1u << 29)) || toggleDown))
        {
            if (message == WM_SYSKEYUP) InterlockedExchange(&toggleDown, 0);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&toggleDown, 1, 0)) InterlockedExchange(&toggleRequested, 1);
            return 0;
        }
        if (mode != 0 && window == Window() && message == WM_CLOSE)
        {
            // The native message pump consumes WM_QUIT and unwinds the loop;
            // WinMain then destroys its own window and releases its services.
            PostQuitMessage(0);
            return 0;
        }
        return procedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline void Init(uint8_t* nativeRenderer)
    {
        Renderer.SetAddress(nativeRenderer);
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        lastWindowMode = mode ? mode : 1;
        auto builder = hook::pattern("83 EC 10 53 55 8B 6C 24 20 8B D8 8B 45 00 3B 83 50 01 00 00 75 1D 8B 4C 24 24 8A 11 3A 93 4C 01 00 00");
        auto windowed = hook::pattern("89 83 80 01 00 00 EB 2A");
        auto procedure = hook::pattern("8B 44 24 08 8B 54 24 10 83 EC 28 3D 03 02 00 00 53 55 8B 6C 24 3C 56 57 0F 87 A6 02 00 00");
        modeHook = safetyhook::create_mid(builder.get_first(), Request);
        publicationHook = safetyhook::create_mid(windowed.get_first(), Configured);
        procedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);

        // Closing the window quits from gameplay too. The game's shutdown waits until resources released in
        // gameplay are ten presented frames old, and nothing is presented anymore: each pass of that wait counts as one.
        auto shutdown = hook::pattern("8B 0D ? ? ? ? 8B 01 FF 50 14 84 C0 75 0D 8B 0D ? ? ? ? 8B 11 FF 52 10 EB E4");
        auto presented = hook::pattern("C7 87 98 01 00 00 02 00 00 00 8B 0D ? ? ? ? 89 8F 8C 01 00 00");
        PresentedFrames.SetAddress(*presented.get_first<uint32_t*>(12));
        shutdownHook = safetyhook::create_mid(shutdown.get_first(15), [](SafetyHookContext&)
        {
            ++PresentedFrames;
        });
    }
}
