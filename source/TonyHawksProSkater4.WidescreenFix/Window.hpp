#pragma once
#include <minidx8/d3d8.h>
#include "FrameRate.hpp"
#include "../Shared/TonyHawk/Display.hpp"

namespace THPS4Window
{
    inline SafetyHookInline FactoryHook, PresentHook;
    inline D3DPRESENT_PARAMETERS* Parameters = nullptr;
    inline IDirect3DDevice8** Device = nullptr;
    inline uint8_t* Initialized = nullptr;
    inline uint8_t* Lost = nullptr;
    inline char(__cdecl* Reset)() = nullptr;
    inline LONG Pending = 0;
    inline bool Changing = false;
    inline int LastWindowMode = 1;
    inline RECT FramedRect{};
    inline bool HaveFramedRect = false;
    inline UINT FullscreenInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    inline UINT FullscreenRefresh = 0;
    constexpr wchar_t Property[] = L"THPS4.WidescreenFix.Window";

    struct WindowState
    {
        WNDPROC original;
    };

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto state = reinterpret_cast<WindowState*>(GetPropW(window, Property));
        if (!state)
            return DefWindowProcA(window, message, wParam, lParam);
        if (wParam == VK_RETURN && (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP))
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                InterlockedExchange(&Pending, 1);
            return 0;
        }
        // Device reset can synchronously deactivate the window while rebuilding resources.
        if (Changing && message == WM_ACTIVATE)
            return DefWindowProcA(window, message, wParam, lParam);
        auto original = state->original;
        if (message == WM_NCDESTROY)
        {
            RemovePropW(window, Property);
            InterlockedExchange(&Pending, 0);
            delete state;
        }
        return CallWindowProcA(original, window, message, wParam, lParam);
    }

    inline void StoreParameters(const D3DPRESENT_PARAMETERS& value)
    {
        static_assert(sizeof(value) == 52);
        auto source = reinterpret_cast<const uint32_t*>(&value);
        auto target = reinterpret_cast<uint32_t*>(Parameters);
        for (size_t i = 0; i < sizeof(value) / sizeof(uint32_t); ++i)
            target[i] = source[i];
    }

    inline void Place(HWND window, int mode)
    {
        if (mode)
        {
            WindowedModeWrapper::afterCreateWindow();
            if (mode == 1 && HaveFramedRect)
                SetWindowPos(window, HWND_NOTOPMOST, FramedRect.left, FramedRect.top,
                    FramedRect.right - FramedRect.left, FramedRect.bottom - FramedRect.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        else
        {
            MONITORINFO monitor{ sizeof(monitor) };
            if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                return;
            auto style = GetWindowLongA(window, GWL_STYLE);
            SetWindowLongA(window, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                Parameters->BackBufferWidth, Parameters->BackBufferHeight,
                SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        TonyDisplay::SquareCorners(window);
    }

    inline void Toggle()
    {
        if (!Pending || Changing || !Device || !*Device || !*Initialized || *Lost)
            return;
        auto window = Parameters->hDeviceWindow;
        if (!window || GetForegroundWindow() != window || (*Device)->TestCooperativeLevel() != D3D_OK)
            return;
        if (!InterlockedExchange(&Pending, 0))
            return;
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
        {
            FullscreenInterval = saved.FullScreen_PresentationInterval;
            FullscreenRefresh = saved.FullScreen_RefreshRateInHz;
        }
        auto requested = saved;
        requested.Windowed = nextMode != 0;
        requested.FullScreen_RefreshRateInHz = nextMode ? 0 : FullscreenRefresh;
        requested.FullScreen_PresentationInterval = nextMode ? 0 : FullscreenInterval;
        Changing = true;
        WindowedModeWrapper::GameHWND = window;
        TonyDisplay::Window(nextMode, saved.BackBufferWidth, saved.BackBufferHeight);
        Place(window, nextMode);
        StoreParameters(requested);
        *Lost = 1;
        Reset();
        if (!*Lost)
        {
            if (previousMode)
                LastWindowMode = previousMode;
            Place(window, nextMode);
        }
        else
        {
            OutputDebugStringA("THPS4: window mode reset failed; restoring the previous mode.\n");
            TonyDisplay::Window(previousMode, saved.BackBufferWidth, saved.BackBufferHeight);
            StoreParameters(saved);
            SetWindowLongA(window, GWL_STYLE, style);
            SetWindowLongA(window, GWL_EXSTYLE, exStyle);
            SetWindowPos(window, (exStyle & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_NOTOPMOST,
                previousRect.left, previousRect.top, previousRect.right - previousRect.left,
                previousRect.bottom - previousRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            Reset();
            if (*Lost)
                OutputDebugStringA("THPS4: previous mode reset failed; native device recovery remains active.\n");
        }
        Changing = false;
    }

    inline HRESULT WINAPI Present(IDirect3DDevice8* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (Device && device == *Device)
        {
            SyncFrameRateLimit();
            Toggle();
        }
        auto result = PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
        if (result == D3DERR_INVALIDCALL && Device && device == *Device && SUCCEEDED(device->EndScene()))
        {
            // The native frame loop presents before EndScene, which D3D9 rejects.
            result = PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
            device->BeginScene();
        }
        return result;
    }

    inline void Attach()
    {
        if (!Device || !*Device || !*Initialized)
            return;
        if (!PresentHook)
        {
            auto table = *reinterpret_cast<void***>(*Device);
            PresentHook = safetyhook::create_inline(table[15], Present);
        }
        auto window = Parameters->hDeviceWindow;
        if (!PresentHook || !window || GetPropW(window, Property))
            return;
        auto state = new WindowState{ reinterpret_cast<WNDPROC>(GetWindowLongPtrA(window, GWLP_WNDPROC)) };
        if (!SetPropW(window, Property, state))
        {
            delete state;
            return;
        }
        SetLastError(0);
        if (!SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) && GetLastError())
        {
            RemovePropW(window, Property);
            delete state;
        }
    }

    inline int __cdecl CreateDevice()
    {
        auto result = FactoryHook.ccall<int>();
        Attach();
        return result;
    }

    inline void Install(int mode, HMODULE module = GetModuleHandleW(nullptr))
    {
        auto factory = hook::module_pattern(module, "83 EC 30 53 55 56 57 68 DC 00 00 00 E8 ? ? ? ? 33 ED A3");
        auto device = hook::module_pattern(module, "A1 ? ? ? ? 68 ? ? ? ? 50 8B 10 FF 92 80 00 00 00 A1 ? ? ? ? 68 ? ? ? ? 50 8B 08 FF 91 84 00 00 00");
        auto reset = hook::module_pattern(module, "A0 ? ? ? ? 53 33 DB 3A C3 0F 84 ? ? ? ? 38 1D ? ? ? ? 0F 84 ? ? ? ? E8 ? ? ? ? E8");
        auto parameters = hook::module_pattern(module, "89 2D ? ? ? ? 89 0D ? ? ? ? 89 1D ? ? ? ? A3 ? ? ? ? 89 1D ? ? ? ? 89 2D ? ? ? ?");
        if (factory.size() != 1 || device.size() != 1 || reset.size() != 1 || parameters.size() != 1)
        {
            OutputDebugStringA("THPS4: native window/presentation patterns unavailable.\n");
            return;
        }
        Parameters = reinterpret_cast<D3DPRESENT_PARAMETERS*>(*parameters.get_first<uintptr_t>(2) - offsetof(D3DPRESENT_PARAMETERS, Windowed));
        Device = *device.get_first<IDirect3DDevice8**>(1);
        Initialized = *reset.get_first<uint8_t*>(1);
        Lost = *reset.get_first<uint8_t*>(18);
        Reset = reinterpret_cast<decltype(Reset)>(reset.get_first());
        LastWindowMode = mode ? mode : 1;
        FactoryHook = safetyhook::create_inline(factory.get_first(), CreateDevice);
        if (!FactoryHook)
            OutputDebugStringA("THPS4: window factory hook unavailable.\n");
    }
}
