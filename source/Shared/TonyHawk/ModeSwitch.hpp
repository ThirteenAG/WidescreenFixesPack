#pragma once
#include <d3d9.h>
#include <atomic>
#include "Shared/TonyHawk/Display.hpp"

// Alt+Enter for THUG and THUG2: fullscreen and the window mode swap at the same size, through the game's own
// device-lost handling (it releases its resources, resets the device with its presentation parameters and rebuilds them)
namespace TonyModeSwitch
{
    inline void (__cdecl* Recover)() = nullptr;
    inline uint8_t* Lost = nullptr;
    inline D3DPRESENT_PARAMETERS* Parameters = nullptr;
    inline std::atomic<bool> Pending{ false };
    inline int LastWindowMode = 1;
    inline int Width = 0, Height = 0;
    inline RECT FramedRect{};
    inline bool HaveFramedRect = false;
    inline UINT FullscreenRefresh = 0;

    // The game's window procedure: true when the message was Alt+Enter
    inline bool Message(UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (wParam != VK_RETURN || (message != WM_SYSKEYDOWN && message != WM_SYSKEYUP))
            return false;
        if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
            Pending.store(true, std::memory_order_relaxed);
        return true;
    }

    // The game reads the keyboard through DirectInput: Enter pressed with Alt isn't also a confirm
    inline void KeysRead(uint8_t* keys)
    {
        if ((keys[0x38] & 0x80) || (keys[0xB8] & 0x80))
            keys[0x1C] = 0;
    }

    inline void Place(HWND window, int mode)
    {
        if (mode)
        {
            WindowedModeWrapper::afterCreateWindow();
            if (mode == 2)
                WindowedModeWrapper::CenterWindowPosition(Width, Height);
            if (mode == 1 && HaveFramedRect)
                SetWindowPos(window, HWND_NOTOPMOST, FramedRect.left, FramedRect.top, FramedRect.right - FramedRect.left,
                    FramedRect.bottom - FramedRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        else
        {
            MONITORINFO monitor{ sizeof(monitor) };
            if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                return;
            const auto style = GetWindowLongA(window, GWL_STYLE);
            SetWindowLongA(window, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top, Width, Height,
                SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        TonyDisplay::SquareCorners(window);
    }

    // A windowed device is lost when another program changes the display mode or takes the screen in fullscreen. The
    // game only resets after its own focus loss, which a window mode that keeps running never passes on: without
    // this the picture stays frozen
    inline void Presented(HRESULT result)
    {
        if (result == D3DERR_DEVICELOST)
            *Lost = 1;
    }

    // After a presented frame
    inline void Update(IDirect3DDevice9* device, HWND window)
    {
        if (!Pending.load(std::memory_order_relaxed) || !device || !window || GetForegroundWindow() != window ||
            *Lost || device->TestCooperativeLevel() != D3D_OK)
            return;
        Pending.store(false, std::memory_order_relaxed);
        const auto saved = *Parameters;
        const int previousMode = TonyDisplay::Mode;
        const int nextMode = previousMode ? 0 : LastWindowMode;
        const auto style = GetWindowLongA(window, GWL_STYLE);
        const auto exStyle = GetWindowLongA(window, GWL_EXSTYLE);
        RECT previousRect{};
        GetWindowRect(window, &previousRect);
        if (previousMode == 1)
        {
            FramedRect = previousRect;
            HaveFramedRect = true;
        }
        if (!previousMode)
            FullscreenRefresh = saved.FullScreen_RefreshRateInHz;
        auto requested = saved;
        requested.Windowed = nextMode != 0;
        requested.FullScreen_RefreshRateInHz = nextMode ? 0 : FullscreenRefresh;
        WindowedModeWrapper::GameHWND = window;
        TonyDisplay::Window(nextMode, Width, Height);
        Place(window, nextMode);
        *Parameters = requested;
        *Lost = 1;
        Recover();
        if (!*Lost)
        {
            if (nextMode)
                LastWindowMode = nextMode;
            Place(window, nextMode);
            return;
        }
        // The mode was refused (a fullscreen size the monitor doesn't list): back to the previous one
        TonyDisplay::Window(previousMode, Width, Height);
        *Parameters = saved;
        SetWindowLongA(window, GWL_STYLE, style);
        SetWindowLongA(window, GWL_EXSTYLE, exStyle);
        SetWindowPos(window, (exStyle & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_NOTOPMOST, previousRect.left, previousRect.top,
            previousRect.right - previousRect.left, previousRect.bottom - previousRect.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        Recover();
    }

    // recover: the routine that resets the device while the lost flag is set (flag and parameters as operands)
    inline void Install(const char* recover, size_t lostOffset, size_t parametersOffset, int mode, int width, int height)
    {
        auto pattern = hook::pattern(recover);
        Recover = reinterpret_cast<decltype(Recover)>(pattern.get_first());
        Lost = *pattern.get_first<uint8_t*>(lostOffset);
        Parameters = *pattern.get_first<D3DPRESENT_PARAMETERS*>(parametersOffset);
        LastWindowMode = mode ? mode : 1;
        Width = width;
        Height = height;
    }
}
