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
    inline LONG toggleDown = 0;
    inline std::atomic<HWND> toggleRequested{ nullptr };
    inline bool haveFramedRect = false;
    inline RECT framedRect{};
    inline int framedWidth = 0, framedHeight = 0;
    inline bool placingWindow = false, sizingWindow = false, resizeRequested = false;
    inline SafetyHookInline procedureHook;
    struct ModeChange
    {
        HWND window{};
        IDirect3DDevice9* device{};
        int target{}, width{}, height{};
        UINT refresh{};
        bool recovering{};
        RECT resizedFrame{};
    };
    inline ModeChange pendingMode;
    inline std::atomic<D3DPRESENT_PARAMETERS*> stagedReset{ nullptr };
    inline bool stagedRecovery = false;
    inline std::function<bool(ModeChange&)> queueMode;
    inline std::function<void(const ModeChange&)> commitMode;
    inline std::function<void()> onCreate;
    inline std::function<void()> onDeactivate;
    inline std::function<void(int, int)> publishDimensions;

    inline HWND Window() { return hWnd.is_initialized() ? hWnd.get() : nullptr; }

    inline void CancelToggle(HWND window)
    {
        toggleRequested.compare_exchange_strong(window, nullptr);
    }

    inline void CancelPendingMode(HWND window)
    {
        if (window && pendingMode.window == window && pendingMode.target != mode)
            pendingMode.recovering = true;
    }

    inline bool ToggleKeysDown()
    {
        return (GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(VK_RETURN) & 0x8000);
    }

    inline bool Dimensions(int windowMode, int& width, int& height)
    {
        if (windowMode == 2)
        {
            const auto rect = WindowStyle::MonitorRect(Window());
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
        return width > 0 && height > 0 && width <= 16384 && height <= 16384;
    }

    inline bool Dimensions(int& width, int& height) { return Dimensions(mode, width, height); }

    inline bool Place(int width, int height)
    {
        const auto window = Window();
        if (!window || mode == 0 || !Dimensions(width, height))
            return false;
        const DWORD frameBits = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frameBits | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_SYSMENU;
        const bool previous = std::exchange(placingWindow, true);
        if (style != DWORD(GetWindowLongW(window, GWL_STYLE)))
        {
            SetWindowLongW(window, GWL_STYLE, LONG(style));
            SetWindowPos(window, nullptr, 0, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        WindowStyle::Place(window, width, height);
        if (mode == 1 && haveFramedRect && width == framedWidth && height == framedHeight &&
            !WindowStyle::CoversMonitor(WindowStyle::MonitorRect(window), width, height))
            SetWindowPos(window, nullptr, framedRect.left, framedRect.top,
                framedRect.right - framedRect.left, framedRect.bottom - framedRect.top,
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
        placingWindow = previous;
        return true;
    }

    inline void StageModeReset(D3DPRESENT_PARAMETERS* present)
    {
        if (GetForegroundWindow() != pendingMode.window)
            CancelPendingMode(pendingMode.window);
        stagedRecovery = pendingMode.recovering;
        stagedReset.store(present);
        if (!stagedRecovery)
        {
            present->Windowed = pendingMode.target != 0;
            present->BackBufferWidth = pendingMode.width;
            present->BackBufferHeight = pendingMode.height;
            present->FullScreen_RefreshRateInHz = present->Windowed ? 0 : pendingMode.refresh;
        }
        else
        {
            present->Windowed = mode != 0;
            if (present->Windowed)
                present->FullScreen_RefreshRateInHz = 0;
        }
    }

    // Only the game's own device, not devices other plugins create for their own windows
    inline void Windowed(D3DPRESENT_PARAMETERS* present, HWND focus)
    {
        const auto window = Window();
        if (present == stagedReset.load() || !window || (focus && focus != window && present->hDeviceWindow != window))
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

    inline void FinishModeReset(HRESULT result)
    {
        stagedReset.store(nullptr);
        if (!pendingMode.window || result == D3DERR_DEVICELOST || result == D3DERR_DEVICENOTRESET)
            return;
        if (result != D3D_OK)
        {
            if (!pendingMode.recovering)
                OutputDebugStringA("DriverParallelLines: window mode reset refused; retrying the active mode.\n");
            pendingMode.recovering = true;
            return;
        }
        const auto applied = std::exchange(pendingMode, {});
        // A synchronous Reset can dispatch focus messages. Match the parameters that it used.
        if (applied.window != Window())
            return;
        const bool resized = mode == 1 && applied.target == mode;
        if (stagedRecovery)
        {
            if (resized) Place(BackbufferWidth, BackbufferHeight);
            return;
        }
        if (resized)
        {
            framedRect = applied.resizedFrame;
            framedWidth = applied.width;
            framedHeight = applied.height;
            haveFramedRect = true;
        }
        if (mode) lastWindowMode = mode;
        mode = applied.target;
        if (commitMode) commitMode(applied);
        if (mode == 0) FullscreenStyle();
        else Place(applied.width, applied.height);
    }

    inline void QueueResize(HWND window, IDirect3DDevice9* device)
    {
        if (mode != 1 || !resizeRequested || sizingWindow || pendingMode.window || !device || !queueMode || IsIconic(window))
            return;
        RECT client{}, frame{};
        if (!GetClientRect(window, &client) || !GetWindowRect(window, &frame)) return;
        int width = client.right, height = client.bottom;
        if (!Dimensions(width, height) || (width == BackbufferWidth && height == BackbufferHeight))
        {
            resizeRequested = false;
            return;
        }
        ModeChange next{ window, device, mode, width, height };
        next.resizedFrame = frame;
        if (device->TestCooperativeLevel() == D3D_OK && queueMode(next))
        {
            pendingMode = next;
            resizeRequested = false;
        }
    }

    inline bool Frame(IDirect3DDevice9* device, bool blocked)
    {
        const auto window = Window();
        if (pendingMode.window && (pendingMode.window != window || pendingMode.device != device))
            pendingMode = {};
        if (window && !toggleRequested.load()) QueueResize(window, device);
        if (!window || GetForegroundWindow() != window)
        {
            InterlockedExchange(&toggleDown, ToggleKeysDown());
            toggleRequested.store(nullptr);
            CancelPendingMode(window);
            return false;
        }
        auto requested = toggleRequested.load();
        if (requested && requested != window)
            CancelToggle(requested);
        const bool down = ToggleKeysDown();
        if (down)
        {
            if (!InterlockedCompareExchange(&toggleDown, 1, 0) && !blocked)
                toggleRequested.store(window);
        }
        else InterlockedExchange(&toggleDown, 0);
        const bool suppressInput = down || toggleRequested.load() == window;
        if (blocked)
        {
            CancelToggle(window);
            return suppressInput;
        }
        if (toggleRequested.load() != window || pendingMode.window || !device ||
            device->TestCooperativeLevel() != D3D_OK || !queueMode)
            return suppressInput;
        requested = window;
        if (!toggleRequested.compare_exchange_strong(requested, nullptr) || GetForegroundWindow() != window)
            return suppressInput;
        RECT client{};
        GetClientRect(window, &client);
        if (mode == 1)
        {
            haveFramedRect = GetWindowRect(window, &framedRect) != FALSE;
            framedWidth = client.right;
            framedHeight = client.bottom;
        }
        const int target = mode ? 0 : lastWindowMode;
        const bool restoreFrame = target == 1 && haveFramedRect;
        ModeChange next{ window, device, target, restoreFrame ? framedWidth : 0, restoreFrame ? framedHeight : 0 };
        if (queueMode(next)) pendingMode = next;
        return suppressInput;
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NCCREATE && onCreate) onCreate();
        if (message == WM_KILLFOCUS || message == WM_NCDESTROY ||
            (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE))
        {
            CancelToggle(window);
            if (window == Window())
            {
                if (message == WM_NCDESTROY) sizingWindow = resizeRequested = false;
                CancelPendingMode(window);
                InterlockedExchange(&toggleDown, ToggleKeysDown());
                if (onDeactivate) onDeactivate();
            }
        }
        if (mode == 1 && window == Window())
        {
            if (message == WM_ENTERSIZEMOVE) sizingWindow = true;
            if (message == WM_EXITSIZEMOVE) sizingWindow = false;
            if (message == WM_SIZE)
            {
                if (wParam == SIZE_MINIMIZED) return 0;
                if (!placingWindow) resizeRequested = true;
            }
            if (message == WM_GETMINMAXINFO)
            {
                const auto result = procedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
                const auto frame = WindowStyle::FrameOf(window);
                auto& info = *reinterpret_cast<MINMAXINFO*>(lParam);
                info.ptMinTrackSize.x = std::max(info.ptMinTrackSize.x, 320 + frame.Width());
                info.ptMinTrackSize.y = std::max(info.ptMinTrackSize.y, 240 + frame.Height());
                return result;
            }
        }
        if (window == Window() && wParam == VK_RETURN &&
            (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && ((lParam & (1u << 29)) || toggleDown))
        {
            if (message == WM_SYSKEYUP) InterlockedExchange(&toggleDown, 0);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&toggleDown, 1, 0)) toggleRequested.store(window);
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

    inline void Init()
    {
        mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        lastWindowMode = mode ? mode : 1;
        auto procedure = hook::pattern("55 8B EC 83 EC 20 8B 45 0C 48 48 56 57 0F 84 ? ? ? ? 83 E8 03 74 6F");
        procedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);
        D3D9Options::presentParameters = Windowed;
        D3D9Options::HookCreation();
    }
}
