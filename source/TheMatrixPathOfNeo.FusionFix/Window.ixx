module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "include/WindowStyle.hpp"


export module Window;

import ComVars;
import Layout;
import Renderer;

export namespace NeoWindow
{
    inline SafetyHookInline ProcedureHook;
    inline WNDPROC NativeProcedure = nullptr;
    inline std::atomic<int> Mode{ 0 };
    inline std::atomic<int> AppliedMode{ 0 };
    inline int LastWindowMode = 1;
    inline std::atomic<HWND> PlacedWindow{ nullptr };
    inline thread_local bool Placing = false;

    inline UINT WINAPI ProfileInt(LPCSTR section, LPCSTR key, INT fallback, LPCSTR file)
    {
        if (section && key && _stricmp(section, "RENDERING") == 0 && _stricmp(key, "FULLSCREEN") == 0)
            return Mode == 0 ? 1 : 0;
        return GetPrivateProfileIntA(section, key, fallback, file);
    }

    inline void UpdateSize(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;
        Screen.nWidth = width;
        Screen.nHeight = height;
        Screen.fWidth = static_cast<float>(width);
        Screen.fHeight = static_cast<float>(height);
        Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
        Screen.fHudScale = Screen.fAspectRatio / NeoLayout::DesignAspect;
        Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * NeoLayout::DesignAspect) * 0.5f;
        Screen.nAspectRatioDiff = static_cast<int>(Screen.fWidth * Screen.fHudScale);
    }

    inline void Place(HWND window, bool force)
    {
        const int mode = AppliedMode.load();
        if (!window || mode == 0 || Placing || IsIconic(window) ||
            GetWindowThreadProcessId(window, nullptr) != GetCurrentThreadId())
            return;

        // The game can't resize its device, so the frame has no sizing border
        const DWORD style = mode == 1 ? WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX) : WS_POPUP;
        const DWORD extended = WS_EX_APPWINDOW;
        const auto monitor = WindowStyle::MonitorRect(window);
        RECT client{};
        POINT origin{};
        GetClientRect(window, &client);
        ClientToScreen(window, &origin);
        const bool aligned = !WindowStyle::CoversMonitor(monitor, Screen.nWidth, Screen.nHeight) ||
            (origin.x == monitor.left && origin.y == monitor.top);
        const DWORD current = static_cast<DWORD>(GetWindowLongW(window, GWL_STYLE)) & ~(WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
        if (!force && current == style && client.right == Screen.nWidth && client.bottom == Screen.nHeight && aligned)
            return;

        Placing = true;
        SetWindowLongW(window, GWL_STYLE, style | WS_VISIBLE);
        SetWindowLongW(window, GWL_EXSTYLE, extended);
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
        WindowStyle::Place(window, Screen.nWidth, Screen.nHeight);
        PlacedWindow = window;
        Placing = false;
    }

    inline BOOL WINAPI Position(HWND window, HWND insertAfter, int x, int y, int width, int height, UINT flags)
    {
        // The engine reapplies its saved outer rectangle after creating or resetting the device.
        if (window && window == NeoRenderer::Window.load() && AppliedMode != 0 && !(flags & SWP_NOSIZE) &&
            Screen.nWidth > 0 && Screen.nHeight > 0)
        {
            const auto bounds = WindowStyle::WindowRect(WindowStyle::MonitorRect(window), Screen.nWidth, Screen.nHeight,
                WindowStyle::FrameOf(window));
            x = bounds.left;
            y = bounds.top;
            width = bounds.right - bounds.left;
            height = bounds.bottom - bounds.top;
        }
        return SetWindowPos(window, insertAfter, x, y, width, height, flags);
    }

    inline LRESULT Toggle(HWND window, LPARAM lParam)
    {
        Mode = AppliedMode == 0 ? LastWindowMode : 0;
        return ProcedureHook.stdcall<LRESULT>(window, WM_COMMAND, 40003, lParam);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT handled = 0;
        if (NeoRenderer::Message(window, message, wParam, lParam, handled))
            return handled;
        constexpr UINT dpiChanged = 0x02E0; // WM_DPICHANGED is hidden when targeting Windows XP.
        if (message == dpiChanged && AppliedMode != 0)
        {
            Place(window, true);
            return 0;
        }
        if (message == WM_ACTIVATEAPP && !wParam)
            CursorConfinement::Release();
        if (message == WM_COMMAND && LOWORD(wParam) == 40003)
            return Toggle(window, lParam);
        if (message == WM_SYSKEYDOWN && wParam == VK_RETURN)
        {
            if (!(lParam & (1u << 30)))
                Toggle(window, 0);
            return 0;
        }
        if ((message == WM_SYSKEYUP || message == WM_SYSCHAR) && wParam == VK_RETURN)
            return 0;
        if (message == WM_NCDESTROY)
        {
            auto current = window;
            if (NeoRenderer::Window.compare_exchange_strong(current, nullptr))
                CursorConfinement::Release();
            current = window;
            PlacedWindow.compare_exchange_strong(current, nullptr);
        }
        return ProcedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline bool IsGameWindow(HWND window)
    {
        return window && reinterpret_cast<WNDPROC>(GetClassLongPtrA(window, GCLP_WNDPROC)) == NativeProcedure;
    }

    inline void Parameters(D3DPRESENT_PARAMETERS* parameters, HWND window)
    {
        const HWND target = window ? window : std::exchange(NeoRenderer::ResetWindow, nullptr);
        if (!IsGameWindow(target))
            return;
        parameters->Windowed = Mode != 0;
        if (parameters->Windowed)
        {
            parameters->FullScreen_RefreshRateInHz = 0;
            parameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        }
    }

    inline void DeviceReady(IDirect3DDevice9* device)
    {
        const HWND window = NeoRenderer::DeviceWindow(device);
        if (!IsGameWindow(window))
            return;

        IDirect3DSurface9* backBuffer = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)))
            return;
        D3DSURFACE_DESC description{};
        const HRESULT result = backBuffer->GetDesc(&description);
        backBuffer->Release();
        if (FAILED(result) || !description.Width || !description.Height)
            return;

        UpdateSize(description.Width, description.Height);
        AppliedMode = Mode.load();
        NeoRenderer::Window = window;
        if (AppliedMode != 0)
            Place(window, true);
        else
            WindowStyle::SquareCorners(window);
    }

    inline void AfterReset(IDirect3DDevice9* device)
    {
        if (NeoRenderer::OwnsDeviceWindow(device, NeoRenderer::Window.load()))
            DeviceReady(device);
    }

    inline void Frame()
    {
        const auto window = NeoRenderer::Window.load();
        Place(window, window != PlacedWindow.load());
    }

    inline void Install()
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        const int startupMode = Mode.load();
        LastWindowMode = startupMode == 0 ? 1 : startupMode;
        auto procedure = hook::pattern("8B 0D ? ? ? ? 8B 01 FF 60 28 90 90 90 90 90");
        auto* target = procedure.get_first();
        NativeProcedure = reinterpret_cast<WNDPROC>(target);
        ProcedureHook = safetyhook::create_inline(target, Procedure, SafetyHookInline::StartDisabled);
        (void)ProcedureHook.enable();
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("GetPrivateProfileIntA", ProfileInt));
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("SetWindowPos", Position));
        D3D9Options::presentParameters = Parameters;
        D3D9Options::afterReset = AfterReset;
        NeoRenderer::DeviceCreated = DeviceReady;
        WFP::onGameProcessEvent() += Frame;
    }
}
