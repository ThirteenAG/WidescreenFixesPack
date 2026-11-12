module;

#include <stdafx.h>
#include "FixSettings.hpp"

// Window modes on the game's own windowed switch (the "-windowed" command line): framed with exactly the chosen
// client size, or borderless covering the monitor; the game keeps running in the background when asked.

export module Window;

import ComVars;

export namespace SniperWindow
{
    inline int Mode{}, Width{}, Height{};
    inline decltype(&CreateWindowExA) NativeCreateWindow{};
    inline decltype(&MoveWindow) NativeMoveWindow{};
    inline SafetyHookMid WindowedSwitch;
    inline SafetyHookInline WindowProcHook;
    inline std::atomic<HWND> Window{ nullptr };
    inline bool (*Message)(HWND, UINT, WPARAM, LPARAM, LRESULT&){};

    inline RECT Monitor(HWND window)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor))
            monitor.rcMonitor = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        return monitor.rcMonitor;
    }

    // The outer rectangle whose client area has the chosen size, centred; at the monitor's size the frame lies outside it
    inline RECT Placement(DWORD style, DWORD extended)
    {
        const auto monitor = Monitor(nullptr);
        if (Mode == 2)
            return monitor;
        RECT rectangle{ 0, 0, Width, Height };
        using Adjust = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using Dpi = UINT(WINAPI*)();
        const auto user = GetModuleHandleW(L"user32.dll");
        const auto adjust = reinterpret_cast<Adjust>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
        const auto dpi = reinterpret_cast<Dpi>(GetProcAddress(user, "GetDpiForSystem"));
        if (!adjust || !dpi || !adjust(&rectangle, style, FALSE, extended, dpi()))
            AdjustWindowRectEx(&rectangle, style, FALSE, extended);
        OffsetRect(&rectangle, monitor.left + (monitor.right - monitor.left - Width) / 2,
            monitor.top + (monitor.bottom - monitor.top - Height) / 2);
        return rectangle;
    }

    // Centres the window with exactly the chosen client size, measured on the real frame
    inline void Fit(HWND window)
    {
        RECT client{}, outer{};
        if (Mode != 1 || !GetClientRect(window, &client) || !GetWindowRect(window, &outer))
            return;
        const auto monitor = Monitor(window);
        const int fullWidth = (outer.right - outer.left) + Width - client.right;
        const int fullHeight = (outer.bottom - outer.top) + Height - client.bottom;
        NativeMoveWindow(window, monitor.left + (monitor.right - monitor.left - fullWidth) / 2,
            monitor.top + (monitor.bottom - monitor.top - fullHeight) / 2, fullWidth, fullHeight, TRUE);
    }

    // The game sizes its window with a frame measured at 100% scaling, which shrinks the picture on scaled displays
    inline BOOL WINAPI MoveGameWindow(HWND window, int x, int y, int width, int height, BOOL repaint)
    {
        if (Mode && window == Window.load(std::memory_order_acquire))
        {
            if (Mode == 1)
                Fit(window);
            return TRUE;
        }
        return NativeMoveWindow(window, x, y, width, height, repaint);
    }

    inline HWND WINAPI CreateGameWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const bool game = Mode && !parent && name && !IS_INTRESOURCE(name) && !strcmp(name, "Sniper Elite");
        if (game)
        {
            style = (Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX) | WS_VISIBLE;
            extended &= ~(WS_EX_TOPMOST | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
            const auto rectangle = Placement(style, extended);
            x = rectangle.left;
            y = rectangle.top;
            width = rectangle.right - rectangle.left;
            height = rectangle.bottom - rectangle.top;
        }
        const auto result = NativeCreateWindow(extended, name, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (game && result)
        {
            using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            static const auto setAttribute = []() -> SetAttribute
            {
                const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
            }();
            const DWORD square = 1;
            if (setAttribute)
                setAttribute(result, 33, &square, sizeof(square));

            Window.store(result, std::memory_order_release);
            Fit(result);
        }
        return result;
    }

    inline bool ActivationPending = false;
    constexpr UINT_PTR ActivationTimer = 0x5E1D;

    inline bool DeviceLost()
    {
        using TestCooperativeLevel = HRESULT(__stdcall*)(void*);
        const auto device = D3DDevice.get();
        return device && (*reinterpret_cast<TestCooperativeLevel**>(device))[3](device) == HRESULT(0x88760868); // D3DERR_DEVICELOST
    }

    // The game pauses (and turns its sound down) when it loses the focus; with the setting off it keeps running
    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Window.store(window, std::memory_order_release);
        LRESULT handled = 0;
        if (Message && Message(window, message, wParam, lParam, handled))
            return handled;
        if (message == WM_ACTIVATEAPP && !wParam && !FixSettings().Bool("MAIN.PauseOnFocusLoss"))
            return 0;
        // Fullscreen: on activation the game waits inside this message until its lost device can be reset, but the
        // device only comes back while messages are handled; it hung when the focus left again meanwhile. The
        // activation reaches the game once the device is no longer lost.
        if (Mode == 0 && message == WM_ACTIVATEAPP)
        {
            ActivationPending = wParam && DeviceLost();
            if (ActivationPending)
            {
                SetTimer(window, ActivationTimer, 100, nullptr);
                return 0;
            }
            KillTimer(window, ActivationTimer);
        }
        if (message == WM_TIMER && wParam == ActivationTimer)
        {
            if (!ActivationPending || !DeviceLost())
            {
                KillTimer(window, ActivationTimer);
                if (ActivationPending)
                {
                    ActivationPending = false;
                    return WindowProcHook.stdcall<LRESULT>(window, WM_ACTIVATEAPP, TRUE, 0);
                }
            }
            return 0;
        }
        return WindowProcHook.stdcall<LRESULT>(window, message, wParam, lParam);
    }

    inline void Init(int width, int height)
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        Width = width;
        Height = height;
        auto setup = hook::pattern("8A 15 ? ? ? ? 8B 0D ? ? ? ? 84 D2 8B 15 ? ? ? ? 0F 94 C0");
        auto procedure = hook::pattern("83 EC 54 A1 ? ? ? ? 89 44 24 50");
        WindowedMode.SetAddress(*setup.get_first<uint8_t*>(2));
        auto reset = hook::pattern("A1 ? ? ? ? 8B 10 50 FF 52 0C 3B C3 7C");
        D3DDevice.SetAddress(*reset.get_first<void**>(1));
        WindowedSwitch = safetyhook::create_mid(setup.get_first(), [](SafetyHookContext&)
        {
            WindowedMode = uint8_t(Mode != 0);
        });
        WindowProcHook = safetyhook::create_inline(procedure.get_first(), WindowProc);
        NativeMoveWindow = MoveWindow;
        auto original = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("CreateWindowExA", CreateGameWindow),
            std::forward_as_tuple("MoveWindow", MoveGameWindow));
        NativeCreateWindow = reinterpret_cast<decltype(NativeCreateWindow)>(original.at("CreateWindowExA").get());
    }
}
