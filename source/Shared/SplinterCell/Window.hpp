#pragma once

namespace SCWindow
{
    inline void EnableDpi()
    {
        auto user32 = GetModuleHandleW(L"user32.dll");
        using SetProcessContext = BOOL(WINAPI*)(HANDLE);
        using SetThreadContext = HANDLE(WINAPI*)(HANDLE);
        auto process = reinterpret_cast<SetProcessContext>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (!process || !process(reinterpret_cast<HANDLE>(-4)))
        {
            auto thread = reinterpret_cast<SetThreadContext>(GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
            if (thread)
                thread(reinterpret_cast<HANDLE>(-4));
            else
                SetProcessDPIAware();
        }
    }

    inline MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info = { sizeof(info) };
        GetMonitorInfo(window ? MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    // The frame around the client area of the window with this style, at the DPI of its monitor
    inline RECT Frame(HWND window, LONG style, LONG exStyle)
    {
        using GetDpiForWindowFn = UINT(WINAPI*)(HWND);
        using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        static auto user32 = GetModuleHandleW(L"user32.dll");
        static auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFn>(GetProcAddress(user32, "GetDpiForWindow"));
        static auto adjustWindowRectExForDpi = reinterpret_cast<AdjustWindowRectExForDpiFn>(GetProcAddress(user32, "AdjustWindowRectExForDpi"));
        RECT frame = {};
        if (getDpiForWindow && adjustWindowRectExForDpi)
            adjustWindowRectExForDpi(&frame, style, FALSE, exStyle, getDpiForWindow(window));
        else
            AdjustWindowRectEx(&frame, style, FALSE, exStyle);
        return frame;
    }

    // No rounded corners on Windows 11 (DWMWA_WINDOW_CORNER_PREFERENCE = DWMWCP_DONOTROUND), nothing happens before it
    inline void SquareCorners(HWND window)
    {
        using SetAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttributeFn
        {
            auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return dwm ? reinterpret_cast<SetAttributeFn>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        int preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    // Borderless covers the monitor. A window is centered in the work area when it fits, otherwise its client area is centered
    // on the monitor and the frame goes past the edges (at the desktop resolution it covers the screen like borderless).
    inline void Place(HWND window, int width, int height, bool bBorderless)
    {
        // fullscreen minimizes the window when it isn't active (leaving it with Alt+Enter does that too)
        if (IsIconic(window))
            ShowWindow(window, SW_RESTORE);
        auto info = Monitor(window);
        auto style = GetWindowLong(window, GWL_STYLE) & ~(WS_MINIMIZE | WS_MAXIMIZE);
        auto exStyle = GetWindowLong(window, GWL_EXSTYLE);
        RECT rect = info.rcMonitor;
        if (bBorderless)
        {
            style = (style & ~(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)) | WS_POPUP;
            exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
        }
        else
        {
            style = (style & ~(WS_POPUP | WS_MAXIMIZEBOX)) | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME;
            auto frame = Frame(window, style, exStyle);
            auto& work = info.rcWork;
            auto& area = info.rcMonitor;
            auto w = width + (frame.right - frame.left);
            auto h = height + (frame.bottom - frame.top);
            if (w <= work.right - work.left && h <= work.bottom - work.top)
            {
                rect.left = work.left + ((work.right - work.left) - w) / 2;
                rect.top = work.top + ((work.bottom - work.top) - h) / 2;
            }
            else
            {
                rect.left = area.left + ((area.right - area.left) - width) / 2 + frame.left;
                rect.top = area.top + ((area.bottom - area.top) - height) / 2 + frame.top;
            }
            rect.right = rect.left + w;
            rect.bottom = rect.top + h;
        }
        SetWindowLong(window, GWL_STYLE, style);
        SetWindowLong(window, GWL_EXSTYLE, exStyle);
        SetWindowPos(window, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);
        SquareCorners(window);
    }

    inline constexpr wchar_t StateProperty[] = L"FusionFix.SCWindow";
    struct WindowState
    {
        WNDPROC Previous = nullptr;
        int Mode = 0;
        bool Windowed = false;
        int Width = 0, Height = 0;
        bool InSizeMove = false;
    };

    inline LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
    {
        auto state = reinterpret_cast<WindowState*>(GetPropW(window, StateProperty));
        if (!state)
            return DefWindowProcW(window, message, wparam, lparam);
        const auto result = CallWindowProcW(state->Previous, window, message, wparam, lparam);
        state = reinterpret_cast<WindowState*>(GetPropW(window, StateProperty));
        if (!state)
            return result;
        if (state->Windowed && message == WM_STYLECHANGING && wparam == static_cast<WPARAM>(GWL_STYLE))
        {
            auto style = reinterpret_cast<STYLESTRUCT*>(lparam);
            if (state->Mode == 2)
                style->styleNew = (style->styleNew & ~WS_OVERLAPPEDWINDOW) | WS_POPUP;
            else
                style->styleNew = (style->styleNew & ~(WS_POPUP | WS_MAXIMIZEBOX)) | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        }
        if (message == WM_ENTERSIZEMOVE)
            state->InSizeMove = true;
        else if (message == WM_EXITSIZEMOVE)
            state->InSizeMove = false;
        if (state->Windowed && !state->InSizeMove && message == WM_WINDOWPOSCHANGING && state->Width > 0 && state->Height > 0)
        {
            auto position = reinterpret_cast<WINDOWPOS*>(lparam);
            auto info = Monitor(window);
            if (!(position->flags & SWP_NOSIZE))
            {
                if (state->Mode == 2)
                {
                    position->cx = info.rcMonitor.right - info.rcMonitor.left;
                    position->cy = info.rcMonitor.bottom - info.rcMonitor.top;
                }
                else
                {
                    auto frame = Frame(window, GetWindowLong(window, GWL_STYLE), GetWindowLong(window, GWL_EXSTYLE));
                    position->cx = state->Width + frame.right - frame.left;
                    position->cy = state->Height + frame.bottom - frame.top;
                }
            }
            if (!(position->flags & SWP_NOMOVE))
            {
                auto frame = Frame(window, GetWindowLong(window, GWL_STYLE), GetWindowLong(window, GWL_EXSTYLE));
                position->x = info.rcMonitor.left + ((info.rcMonitor.right - info.rcMonitor.left) - state->Width) / 2 + (state->Mode == 2 ? 0 : frame.left);
                position->y = info.rcMonitor.top + ((info.rcMonitor.bottom - info.rcMonitor.top) - state->Height) / 2 + (state->Mode == 2 ? 0 : frame.top);
            }
        }
        if (message == WM_NCDESTROY)
        {
            RemovePropW(window, StateProperty);
            delete state;
        }
        return result;
    }

    inline void Track(HWND window, int mode, bool windowed, int width, int height)
    {
        if (!window)
            return;
        auto state = reinterpret_cast<WindowState*>(GetPropW(window, StateProperty));
        if (!state)
        {
            state = new WindowState;
            state->Previous = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
            if (!state->Previous || !SetPropW(window, StateProperty, state))
            {
                delete state;
                return;
            }
            SetLastError(0);
            if (!SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)) && GetLastError())
            {
                RemovePropW(window, StateProperty);
                delete state;
                return;
            }
        }
        state->Mode = mode;
        state->Windowed = windowed;
        state->Width = width;
        state->Height = height;
    }

}
