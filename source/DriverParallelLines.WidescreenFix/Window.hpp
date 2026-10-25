#pragma once
#include <d3d9.h>
#include "../../external/FusionFixUtils/include/WindowStyle.hpp"
#include "../Shared/D3D9Options.hpp"

// Window mode: the device is created and reset windowed at the selected resolution, and the window is centered on its
// monitor with a frame (Windowed) or without one (Borderless); at the desktop resolution it covers the screen.
namespace DPLWindow
{
    inline HWND* windowSlot = nullptr;
    inline int mode = 0;
    inline bool ready = false;

    inline HWND Window() { return windowSlot ? *windowSlot : nullptr; }

    inline bool Place(int width, int height)
    {
        const auto window = Window();
        if (!ready || !window || width <= 0 || height <= 0 || width > 16384 || height > 16384)
            return false;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::Place(window, width, height);
        return true;
    }

    // Only the game's own device, not devices other plugins create for their own windows
    inline void Windowed(D3DPRESENT_PARAMETERS* present, HWND focus)
    {
        const auto window = Window();
        if (!window || (focus && focus != window && present->hDeviceWindow != window))
            return;
        present->Windowed = TRUE;
        present->FullScreen_RefreshRateInHz = 0;
        Place(int(present->BackBufferWidth), int(present->BackBufferHeight));
    }

    inline void Init(HWND* nativeWindow)
    {
        windowSlot = nativeWindow;
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        if (mode == 0 || !windowSlot)
            return;
        D3D9Options::presentParameters = Windowed;
        ready = D3D9Options::HookCreation();
        if (!ready)
        {
            D3D9Options::presentParameters = nullptr;
            OutputDebugStringA("DriverParallelLines: window mode unavailable; fullscreen retained.\n");
        }
    }
}
