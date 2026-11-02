module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

// Window modes through the game's own device settings: Windowed and Borderless ask the device setup for a windowed
// device; the window is then placed so its client area is exactly the back buffer (centred on its monitor, a client
// as big as the monitor with the frame outside it). Borderless uses a popup window covering the monitor.

export module Window;

import ComVars;


export namespace SecondSightWindow
{
    inline int Mode = 0;
    inline HWND Window = nullptr;
    inline SafetyHookMid SettingsHook;
    inline decltype(&SetWindowPos) OriginalPosition = SetWindowPos;
    inline decltype(&CreateWindowExA) OriginalCreate = CreateWindowExA;
    inline std::function<bool(HWND, UINT, WPARAM, LPARAM, LRESULT&)> Message;
    inline POINT Placed{};
    inline bool HasPlacement = false;

    inline RECT Frame(HWND window, DWORD style, DWORD extended)
    {
        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using GetDpi = UINT(WINAPI*)(HWND);
        static const auto user = GetModuleHandleW(L"user32.dll");
        static const auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
        static const auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user, "GetDpiForWindow"));
        RECT frame{};
        const UINT dpi = getDpi && window ? getDpi(window) : 0;
        if (!adjust || !dpi || !adjust(&frame, style, FALSE, extended, dpi))
            AdjustWindowRectEx(&frame, style, FALSE, extended);
        return frame;
    }

    inline MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &info);
        return info;
    }

    inline void Corners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto setAttribute = []()
        {
            auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
        }();
        const DWORD square = 1;
        if (setAttribute)
            setAttribute(window, 33, &square, sizeof(square));
    }

    // The device setup sizes the window with AdjustWindowRect for its client size; the same client size gets the
    // frame of this window at its own DPI, centred on the monitor. Moves without a size keep that place.
    inline BOOL WINAPI Position(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (Mode && window && window == Window)
        {
            const auto monitor = Monitor(window).rcMonitor;
            const int monitorWidth = monitor.right - monitor.left, monitorHeight = monitor.bottom - monitor.top;
            if (!(flags & SWP_NOSIZE))
            {
                const DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)), extended = DWORD(GetWindowLongW(window, GWL_EXSTYLE));
                if (Mode == 2)
                {
                    x = monitor.left;
                    y = monitor.top;
                    width = monitorWidth;
                    height = monitorHeight;
                }
                else
                {
                    RECT assumed{};
                    AdjustWindowRect(&assumed, style, FALSE);
                    const int clientWidth = width - (assumed.right - assumed.left);
                    const int clientHeight = height - (assumed.bottom - assumed.top);
                    const auto frame = Frame(window, style, extended);
                    width = clientWidth + frame.right - frame.left;
                    height = clientHeight + frame.bottom - frame.top;
                    x = monitor.left + (monitorWidth - clientWidth) / 2 + frame.left;
                    y = monitor.top + (monitorHeight - clientHeight) / 2 + frame.top;
                    if (clientWidth < monitorWidth || clientHeight < monitorHeight)
                    {
                        // A smaller window keeps its title bar on the monitor
                        x = (std::max)(x, int(monitor.left));
                        y = (std::max)(y, int(monitor.top));
                    }
                }
                Placed = { x, y };
                HasPlacement = true;
                flags &= ~SWP_NOMOVE;
                Corners(window);
            }
            else if (HasPlacement && !(flags & SWP_NOMOVE))
            {
                x = Placed.x;
                y = Placed.y;
            }
        }
        return OriginalPosition(window, after, x, y, width, height, flags);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        LRESULT result = 0;
        if (Message && Message(window, message, wParam, lParam, result))
            return result;
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, 0);
            if (window == Window)
                Window = nullptr;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline HWND WINAPI Create(DWORD extended, LPCSTR className, LPCSTR title, DWORD style, int x, int y, int width, int height,
        HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const auto window = OriginalCreate(extended, className, title, style, x, y, width, height, parent, menu, instance, parameter);
        if (window && !parent && reinterpret_cast<uintptr_t>(className) > 0xFFFF && std::strcmp(className, "Direct3DWindowClass") == 0)
        {
            Window = window;
            SetWindowSubclass(window, Procedure, 0, 0);
            if (Mode)
                Corners(window);
        }
        return window;
    }

    // chosen: the game's scr_width, scr_height, scr_hz
    inline void Init(int mode)
    {
        Mode = std::clamp(mode, 0, 2);
        const auto exe = GetModuleHandleW(nullptr);
        auto originals = IATHook::Replace(exe, "USER32.dll",
            std::pair{ "CreateWindowExA", reinterpret_cast<void*>(&Create) },
            std::pair{ "SetWindowPos", reinterpret_cast<void*>(&Position) });
        if (originals.contains("CreateWindowExA"))
            OriginalCreate = reinterpret_cast<decltype(OriginalCreate)>(originals.at("CreateWindowExA").get());
        if (originals.contains("SetWindowPos"))
            OriginalPosition = reinterpret_cast<decltype(OriginalPosition)>(originals.at("SetWindowPos").get());
        if (!originals.contains("CreateWindowExA") || !originals.contains("SetWindowPos"))
            OutputDebugStringA("Second Sight: window hooks unavailable.\n");

        // The start-up device settings (D3DPRESENT_PARAMETERS inside) and the game's renderer size get the resolution
        // chosen in the game's options; window modes ask for a windowed device of exactly that size. Borderless shows
        // it scaled to the whole monitor.
        auto settings = hook::pattern("8D 4C 24 74 51 8D 94 24 C0 00 00 00 52 E8");
        auto size = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 89 5C 24 4C");
        auto storedStyle = hook::pattern("68 00 00 CF 00 E8 ? ? ? ? 8B C8 E8");
        auto createStyle = hook::pattern("68 00 00 CF 00 68 ? ? ? ? 68 ? ? ? ? 6A 00 FF 15");
        DeviceWidth.SetAddress(*size.get_first<uint32_t*>(1));
        DeviceHeight.SetAddress(*size.get_first<uint32_t*>(7));
        DeviceRefresh.SetAddress(DeviceHeight.get_ptr() + 1);
        SettingsHook = safetyhook::create_mid(settings.get_first(13), [](SafetyHookContext& context)
        {
            auto device = *reinterpret_cast<uint32_t**>(context.esp + 4);
            auto options = *reinterpret_cast<uint32_t**>(context.esp + 8);
            if (ChosenWidth.is_initialized())
            {
                device[4] = ChosenWidth.get();
                device[5] = ChosenHeight.get();
                device[16] = ChosenRefresh.get();
                DeviceWidth = ChosenWidth.get();
                DeviceHeight = ChosenHeight.get();
                DeviceRefresh = ChosenRefresh.get();
            }
            if (Mode)
            {
                device[12] = TRUE; // Windowed
                device[16] = 0;    // FullScreen_RefreshRateInHz
                options[5] = 2;    // this size for the window, also when no display mode has it
                options[13] = 0;   // the game asks to keep its refresh rate, which no windowed device has
            }
        });
        if (Mode == 2)
        {
            injector::WriteMemory<uint32_t>(storedStyle.get_first<uint32_t>(1), WS_POPUP, true);
            injector::WriteMemory<uint32_t>(createStyle.get_first<uint32_t>(1), WS_POPUP, true);
        }
    }
}
