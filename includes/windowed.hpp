#pragma once
#include <windows.h>
#include <algorithm>
#include <tuple>
#include <cstdint>
#include "include/WindowStyle.hpp"

// Windowed and borderless modes for games made for full screen: the fixes route the game's window calls through these
// hooks. Windowed keeps a caption (resizable when asked) and centres the client area on the game's monitor; borderless
// strips the frame and keeps the window centred whatever the game does to it.
namespace WindowedModeWrapper
{
    inline bool bBorderlessWindowed{ true };
    inline bool bEnableWindowResize{ false };
    inline bool bScaleWindow{ false };      // as large as the monitor allows at the game's aspect ratio
    inline bool bStretchWindow{ false };    // the whole monitor
    inline HWND GameHWND{ nullptr };
    inline int desiredClientWidth{ 0 };
    inline int desiredClientHeight{ 0 };
    inline bool s_cursorOnNcArea{ false };

    namespace detail
    {
        template<typename Function>
        Function User32(const char* name)
        {
            static const HMODULE user32 = GetModuleHandleW(L"user32.dll");
            return user32 ? reinterpret_cast<Function>(GetProcAddress(user32, name)) : nullptr;
        }

        constexpr LONG FramedStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
        constexpr LONG ResizableStyle = WS_MAXIMIZEBOX | WS_THICKFRAME;
        constexpr LONG FrameStyle = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;

        // The style the game's window gets in the chosen mode, from the one it has or asks for
        inline LONG ModeStyle(LONG style)
        {
            style &= ~(WS_HSCROLL | WS_VSCROLL);
            if (bBorderlessWindowed)
                return style & ~FrameStyle;
            style |= FramedStyle;
            if (bEnableWindowResize)
                style |= ResizableStyle;
            return style;
        }

        struct Placement
        {
            int x, y, width, height;
        };
    }

    // Per-monitor DPI awareness, so Windows doesn't scale the game's picture (system awareness before Windows 10 1703)
    static void EnableDpiAwareness()
    {
        const auto perMonitorV2 = reinterpret_cast<HANDLE>(-4); // DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
        if (auto process = detail::User32<BOOL(WINAPI*)(HANDLE)>("SetProcessDpiAwarenessContext"); process && process(perMonitorV2))
            return;
        if (auto thread = detail::User32<HANDLE(WINAPI*)(HANDLE)>("SetThreadDpiAwarenessContext"))
            thread(perMonitorV2);
        else
            SetProcessDPIAware();
    }

    // AdjustWindowRectEx at the DPI of the game's window (or the system's), where Windows has it
    static BOOL AdjustFrame(LPRECT rect, DWORD style, BOOL menu, DWORD exStyle)
    {
        const auto adjustForDpi = detail::User32<BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT)>("AdjustWindowRectExForDpi");
        if (adjustForDpi)
            return adjustForDpi(rect, style, menu, exStyle, WindowStyle::DpiOf(GameHWND));
        return AdjustWindowRectEx(rect, style, menu, exStyle);
    }

    // The monitor the game's window is on, else the primary one, else the screen size Windows reports
    static MONITORINFOEX WindowMonitor()
    {
        MONITORINFOEX info{};
        info.cbSize = sizeof(info);
        const HWND window = (GameHWND && IsWindow(GameHWND)) ? GameHWND : nullptr;
        const HMONITOR primary = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
        const HMONITOR monitor = window ? MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST) : primary;
        if (GetMonitorInfo(monitor, &info) || GetMonitorInfo(primary, &info))
            return info;
        info.rcMonitor = { 0, 0, (std::max)(GetSystemMetrics(SM_CXSCREEN), 640), (std::max)(GetSystemMetrics(SM_CYSCREEN), 480) };
        info.rcWork = info.rcMonitor;
        return info;
    }

    namespace detail
    {
        // A window of the given size, scaled or stretched to the monitor as chosen, centred on it
        inline Placement Centered(int width, int height)
        {
            const RECT screen = WindowMonitor().rcMonitor;
            const int screenWidth = screen.right - screen.left;
            const int screenHeight = screen.bottom - screen.top;
            if (bStretchWindow)
            {
                width = screenWidth;
                height = screenHeight;
            }
            else if (bScaleWindow && width > 0 && height > 0)
            {
                // Full height, unless that makes it wider than the monitor: then full width
                const double aspect = double(width) / double(height);
                width = int(screenHeight * aspect);
                height = screenHeight;
                if (width > screenWidth)
                {
                    width = screenWidth;
                    height = int(screenWidth / aspect);
                }
            }
            return { screen.left + (screenWidth - width) / 2, screen.top + (screenHeight - height) / 2, width, height };
        }

        // Puts the game's window in the middle of its monitor at its current client size
        inline void Recenter(HWND window)
        {
            RECT client{};
            GetClientRect(window, &client);
            const auto place = Centered(client.right, client.bottom);
            SetWindowPos(GameHWND, nullptr, place.x, place.y, place.width, place.height, SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    // Where the game's window is created and how big
    static std::tuple<int, int, int, int> beforeCreateWindow(int nWidth, int nHeight)
    {
        EnableDpiAwareness();
        const auto place = detail::Centered(nWidth, nHeight);
        if (bBorderlessWindowed || desiredClientWidth <= 0 || desiredClientHeight <= 0)
        {
            desiredClientWidth = place.width;
            desiredClientHeight = place.height;
        }
        return { place.x, place.y, place.width, place.height };
    }

    // Gives the game's window the mode's style; a framed window gets the size that makes its client area the one asked for
    static void afterCreateWindow()
    {
        const LONG style = detail::ModeStyle(GetWindowLong(GameHWND, GWL_STYLE));
        SetWindowLong(GameHWND, GWL_STYLE, style);

        if (!bBorderlessWindowed && desiredClientWidth > 0 && desiredClientHeight > 0)
        {
            RECT frame{ 0, 0, desiredClientWidth, desiredClientHeight };
            AdjustFrame(&frame, style, FALSE, GetWindowLong(GameHWND, GWL_EXSTYLE));
            const RECT screen = WindowMonitor().rcMonitor;
            const int screenWidth = screen.right - screen.left;
            const int screenHeight = screen.bottom - screen.top;
            // Centred by the client area; frame.left/top are the (negative) border sizes
            const int x = screen.left + (screenWidth - desiredClientWidth) / 2 + frame.left;
            const int y = screen.top + (screenHeight - desiredClientHeight) / 2 + frame.top;
            UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED;
            // A client area as big as the monitor puts the frame past its edges; Windows would shrink the window
            if (desiredClientWidth >= screenWidth || desiredClientHeight >= screenHeight)
                flags |= SWP_NOSENDCHANGING;
            SetWindowPos(GameHWND, nullptr, x, y, frame.right - frame.left, frame.bottom - frame.top, flags);
        }
        else
            SetWindowPos(GameHWND, nullptr, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

        // Square corners on Windows 11 (earlier Windows ignore it)
        if (GameHWND)
            WindowStyle::SquareCorners(GameHWND);
    }

    namespace detail
    {
        // The game sizes its window from the client area it wants: windowed remembers that size and frames it with a
        // caption; borderless applies its style first and adds nothing
        inline BOOL AdjustRect(LPRECT rect, BOOL menu, DWORD exStyle)
        {
            if (!bBorderlessWindowed)
            {
                desiredClientWidth = rect->right - rect->left;
                desiredClientHeight = rect->bottom - rect->top;
                return AdjustFrame(rect, WS_CAPTION, menu, exStyle);
            }
            afterCreateWindow();
            return AdjustFrame(rect, GetWindowLong(GameHWND, GWL_STYLE), menu, exStyle);
        }

        // A top-level window gets the mode's position, size and style; child windows are left alone
        template<typename Char, typename Create>
        HWND CreateWindowIn(Create create, DWORD exStyle, const Char* className, const Char* windowName, DWORD style, int x, int y,
            int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
        {
            if (parent && parent != GetDesktopWindow())
                return create(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param);
            const auto [placeX, placeY, placeWidth, placeHeight] = beforeCreateWindow(width, height);
            GameHWND = create(exStyle, className, windowName, style, placeX, placeY, placeWidth, placeHeight, parent, menu, instance, param);
            afterCreateWindow();
            return GameHWND;
        }

        // The game changing its window's style: borderless keeps its own style (and position); windowed keeps the frame
        template<typename SetLong>
        LONG SetStyle(SetLong setLong, HWND window, int index, LONG value)
        {
            if (window != GameHWND || index != GWL_STYLE)
                return setLong(window, index, value);
            if (bBorderlessWindowed)
            {
                const LONG previous = GetWindowLong(window, GWL_STYLE);
                afterCreateWindow();
                Recenter(window);
                return previous;
            }
            value |= FramedStyle;
            if (bEnableWindowResize)
                value |= ResizableStyle;
            return setLong(window, GWL_STYLE, value);
        }
    }

    static BOOL WINAPI AdjustWindowRect_Hook(LPRECT clientRect, DWORD /*style*/, BOOL menu)
    {
        return detail::AdjustRect(clientRect, menu, 0);
    }

    static BOOL WINAPI AdjustWindowRectEx_Hook(LPRECT clientRect, DWORD /*style*/, BOOL menu, DWORD exStyle)
    {
        return detail::AdjustRect(clientRect, menu, exStyle);
    }

    // Moves the game's window to the middle of its monitor at the given client size (scaled or stretched as chosen)
    static BOOL WINAPI CenterWindowPosition(int clientWidth, int clientHeight)
    {
        const auto place = detail::Centered(clientWidth, clientHeight);
        return SetWindowPos(GameHWND, nullptr, place.x, place.y, place.width, place.height, SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    static HWND WINAPI CreateWindowExA_Hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        return detail::CreateWindowIn<char>(CreateWindowExA, dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    static HWND WINAPI CreateWindowExW_Hook(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
    {
        return detail::CreateWindowIn<wchar_t>(CreateWindowExW, dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    }

    static LONG WINAPI SetWindowLongA_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        return detail::SetStyle(SetWindowLongA, hWnd, nIndex, dwNewLong);
    }

    static LONG WINAPI SetWindowLongW_Hook(HWND hWnd, int nIndex, LONG dwNewLong)
    {
        return detail::SetStyle(SetWindowLongW, hWnd, nIndex, dwNewLong);
    }

    // Borderless: whatever the game sets, the window stays in its style in the middle of its monitor
    static BOOL WINAPI SetWindowPos_Hook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
    {
        const BOOL result = SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags);
        if (hWnd != GameHWND || !bBorderlessWindowed)
            return result;
        afterCreateWindow();
        detail::Recenter(hWnd);
        return TRUE;
    }

    static BOOL WINAPI MoveWindow_Hook(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint)
    {
        const BOOL result = MoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
        if (bBorderlessWindowed && hWnd == GameHWND)
            CenterWindowPosition(nWidth, nHeight);
        return result;
    }

    // Windowed: the game hiding the cursor doesn't hide it over the caption and borders, so the window can be moved
    static int WINAPI ShowCursor_Hook(BOOL bShow)
    {
        if (bBorderlessWindowed || !GameHWND || bShow)
            return ShowCursor(bShow);

        POINT cursor{};
        GetCursorPos(&cursor);
        RECT client{};
        GetClientRect(GameHWND, &client);
        MapWindowPoints(GameHWND, nullptr, reinterpret_cast<LPPOINT>(&client), 2);
        if (!PtInRect(&client, cursor))
        {
            if (!s_cursorOnNcArea)
            {
                s_cursorOnNcArea = true;
                while (ShowCursor(TRUE) < 0) {}
            }
            return 0;
        }
        s_cursorOnNcArea = false;
        return ShowCursor(bShow);
    }
}
