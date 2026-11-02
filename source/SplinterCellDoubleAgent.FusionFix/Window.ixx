module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Window;

namespace NativeWindow
{
    MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info = { sizeof(info) };
        GetMonitorInfo(window ? MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY) : MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    // The frame around the client area of the window with this style, at the DPI of its monitor
    RECT Frame(HWND window, LONG style, LONG exStyle)
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
    void SquareCorners(HWND window)
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
    void Place(HWND window, int width, int height)
    {
        // fullscreen minimizes the window when it isn't active (leaving it with Alt+Enter does that too)
        if (IsIconic(window))
            ShowWindow(window, SW_RESTORE);
        auto info = Monitor(window);
        auto style = GetWindowLong(window, GWL_STYLE) & ~(WS_MINIMIZE | WS_MAXIMIZE);
        auto exStyle = GetWindowLong(window, GWL_EXSTYLE);
        RECT rect = info.rcMonitor;
        static const int mode = FixSettings().Int("MAIN.WindowMode");
        if (mode == 2)
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

}

export void PlaceGameWindow(HWND window, int width, int height)
{
    NativeWindow::Place(window, width, height);
}

export std::pair<int, int> GetGameMonitorSize(HWND window)
{
    auto area = NativeWindow::Monitor(window).rcMonitor;
    return { area.right - area.left, area.bottom - area.top };
}

export void InitWindow()
{
    //icon fix
    using RegisterClassW = ATOM(WINAPI*)(WNDCLASSEXW*);
    auto RegisterClassExWProxy = [](WNDCLASSEXW* lpwcx) -> ATOM
    {
        lpwcx->hIcon = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
        lpwcx->hIconSm = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
        return RegisterClassExW(lpwcx);
    };

    using RegisterClassA = ATOM(WINAPI*)(WNDCLASSEXA*);
    auto RegisterClassExAProxy = [](WNDCLASSEXA* lpwcx) -> ATOM
    {
        lpwcx->hIcon = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
        lpwcx->hIconSm = CreateIconFromResourceICO(IDR_SCDAICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
        return RegisterClassExA(lpwcx);
    };

    auto pattern = hook::module_pattern(GetModuleHandle(L"Window"), "FF 15 ? ? ? ? 66 85 C0 0F 85 ? ? ? ? 68 F5 02 00 00");
    injector::WriteMemory<RegisterClassW>(*pattern.get_first<RegisterClassW*>(2), static_cast<RegisterClassW>(RegisterClassExWProxy), true);
    pattern = hook::module_pattern(GetModuleHandle(L"Window"), "FF 15 ? ? ? ? 66 85 C0 75 18 68 03 03 00 00");
    injector::WriteMemory<RegisterClassA>(*pattern.get_first<RegisterClassA*>(2), static_cast<RegisterClassA>(RegisterClassExAProxy), true);
}
