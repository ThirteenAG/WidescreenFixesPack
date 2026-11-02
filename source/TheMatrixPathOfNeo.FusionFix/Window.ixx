module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")


export module Window;

import ComVars;
import Layout;
import Renderer;

export namespace NeoWindow
{
    inline HWND Window = nullptr;
    inline SafetyHookInline ProcedureHook;
    inline int Mode = 0;
    inline int LastWindowMode = 1;
    inline bool Placing = false, Placed = false;

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

    inline void Place()
    {
        if (!Window || Mode == 0 || Placing)
            return;
        Placing = true;
        // The game can't resize its device, so the frame has no sizing border
        const DWORD style = Mode == 1 ? WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX) : WS_POPUP;
        const DWORD extended = WS_EX_APPWINDOW;
        MONITORINFO monitor{ sizeof(monitor) };
        GetMonitorInfoW(MonitorFromWindow(Window, MONITOR_DEFAULTTONEAREST), &monitor);
        RECT rectangle{ 0, 0, Screen.nWidth, Screen.nHeight };
        WindowedModeWrapper::GameHWND = Window;
        WindowedModeWrapper::AdjustFrame(&rectangle, style, FALSE, extended);
        const auto& bounds = Screen.nWidth >= monitor.rcMonitor.right - monitor.rcMonitor.left ||
            Screen.nHeight >= monitor.rcMonitor.bottom - monitor.rcMonitor.top ? monitor.rcMonitor : monitor.rcWork;
        const int x = bounds.left + (bounds.right - bounds.left - Screen.nWidth) / 2;
        const int y = bounds.top + (bounds.bottom - bounds.top - Screen.nHeight) / 2;
        SetWindowLongW(Window, GWL_STYLE, style | WS_VISIBLE);
        SetWindowLongW(Window, GWL_EXSTYLE, extended);
        SetWindowPos(Window, nullptr, x + rectangle.left, y + rectangle.top, rectangle.right - rectangle.left,
            rectangle.bottom - rectangle.top, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
        const DWORD preference = DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(Window, DWMWA_WINDOW_CORNER_PREFERENCE, &preference, sizeof(preference));
        Placing = false;
    }

    inline LRESULT Toggle(HWND window, LPARAM lParam)
    {
        Mode = Mode == 0 ? LastWindowMode : 0;
        Placed = false;
        return ProcedureHook.stdcall<LRESULT>(window, WM_COMMAND, 40003, lParam);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window = window;
        LRESULT handled = 0;
        if (NeoRenderer::Message(window, message, wParam, lParam, handled))
            return handled;
        if (message == 0x02E0 && Mode)
        {
            Place();
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
            CursorConfinement::Release();
            Window = nullptr;
            Placed = false;
        }
        return ProcedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline void Parameters(D3DPRESENT_PARAMETERS* parameters, HWND window)
    {
        if (window)
            NeoRenderer::Window = window;
        parameters->Windowed = Mode != 0;
        if (parameters->Windowed)
        {
            parameters->FullScreen_RefreshRateInHz = 0;
            parameters->PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        }
        UpdateSize(parameters->BackBufferWidth, parameters->BackBufferHeight);
    }

    inline void Frame()
    {
        const auto window = NeoRenderer::Window.load();
        if (window && (!Placed || window != Window))
        {
            Window = window;
            Placed = true;
            Place();
        }
    }

    inline void Install()
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        LastWindowMode = Mode == 0 ? 1 : Mode;
        auto procedure = hook::pattern("8B 0D ? ? ? ? 8B 01 FF 60 28 90 90 90 90 90");
        ProcedureHook = safetyhook::create_inline(procedure.get_first(), Procedure, SafetyHookInline::StartDisabled);
        (void)ProcedureHook.enable();
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("GetPrivateProfileIntA", ProfileInt));
        D3D9Options::presentParameters = Parameters;
        WFP::onGameProcessEvent() += Frame;
    }
}
