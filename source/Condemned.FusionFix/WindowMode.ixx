module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"

export module WindowMode;


export namespace CondemnedWindow
{
    inline int Mode = 0;
    inline decltype(&SetWindowPos) OriginalPosition = SetWindowPos;
    inline SafetyHookMid SelectionHook, ClientSelectionHook;

    inline RECT Frame(HWND window, DWORD style, DWORD extended)
    {
        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using GetDpi = UINT(WINAPI*)(HWND);
        static const auto user = GetModuleHandleW(L"user32.dll");
        static const auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
        static const auto getDpi = reinterpret_cast<GetDpi>(GetProcAddress(user, "GetDpiForWindow"));
        RECT frame{};
        const UINT dpi = getDpi ? getDpi(window) : 0;
        if (!adjust || !dpi || !adjust(&frame, style, FALSE, extended, dpi))
            AdjustWindowRectEx(&frame, style, FALSE, extended);
        return frame;
    }

    // The renderer sizes the window for its client size with the frame of an overlapped window (AdjustWindowRect
    // with WS_OVERLAPPEDWINDOW): that client size gets the frame of the window's real style, centred on its monitor.
    // A client as big as the monitor lies exactly on it with the frame outside. Borderless covers the monitor.
    inline BOOL WINAPI Position(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (Mode && window && !(flags & SWP_NOSIZE))
        {
            MONITORINFO monitor{ sizeof(monitor) };
            if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
            {
                const auto& bounds = monitor.rcMonitor;
                const int monitorWidth = bounds.right - bounds.left, monitorHeight = bounds.bottom - bounds.top;
                if (Mode == 2)
                {
                    x = bounds.left;
                    y = bounds.top;
                    width = monitorWidth;
                    height = monitorHeight;
                }
                else
                {
                    RECT assumed{};
                    AdjustWindowRect(&assumed, WS_OVERLAPPEDWINDOW, FALSE);
                    const int clientWidth = width - (assumed.right - assumed.left);
                    const int clientHeight = height - (assumed.bottom - assumed.top);
                    const auto frame = Frame(window, DWORD(GetWindowLongW(window, GWL_STYLE)), DWORD(GetWindowLongW(window, GWL_EXSTYLE)));
                    width = clientWidth + frame.right - frame.left;
                    height = clientHeight + frame.bottom - frame.top;
                    x = bounds.left + (monitorWidth - clientWidth) / 2 + frame.left;
                    y = bounds.top + (monitorHeight - clientHeight) / 2 + frame.top;
                    if (clientWidth < monitorWidth || clientHeight < monitorHeight)
                    {
                        // A smaller window keeps its title bar on the monitor
                        x = (std::max)(x, int(bounds.left));
                        y = (std::max)(y, int(bounds.top));
                    }
                }
                flags &= ~(SWP_NOMOVE | SWP_NOSIZE);
            }
            using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
            static const auto setAttribute = []()
            {
                auto dwm = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                return dwm ? reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute")) : nullptr;
            }();
            if (setAttribute)
            {
                const DWORD square = 1;
                setAttribute(window, 33, &square, sizeof(square));
            }
        }
        return OriginalPosition(window, after, x, y, width, height, flags);
    }

    inline void Install(int mode)
    {
        auto selection = hook::pattern("8B 0D ? ? ? ? 85 C9 A3 ? ? ? ? A1 ? ? ? ? 0F 95 C3 85 C0 88 5C 24 10");
        auto sizing = hook::pattern("68 00 02 00 00 2B CA 51 2B F0 56 52 8B 15 ? ? ? ? 50 6A 00 52 FF 15");
        auto rectangleStyle = hook::pattern("68 ? ? ? ? 8D 4C 24 0C 03 C7 51");
        auto windowStyle = hook::pattern("68 ? ? ? ? 52 68 ? ? ? ? 53");
        auto positionSlot = *sizing.get_first<decltype(&SetWindowPos)*>(24);
        Mode = std::clamp(mode, 0, 2);
        // Override the renderer's native Windowed decision after configuration is read.
        SelectionHook = safetyhook::create_mid(selection.get_first(6), [](SafetyHookContext& context)
        {
            context.ecx = Mode != 0;
        });
        OriginalPosition = *positionSlot;
        injector::WriteMemory<decltype(&SetWindowPos)>(positionSlot, Position, true);
        if (Mode == 2)
        {
            injector::WriteMemory<uint32_t>(rectangleStyle.get_first<uint32_t>(1), WS_POPUP, true);
            injector::WriteMemory<uint32_t>(windowStyle.get_first<uint32_t>(1), WS_POPUP, true);
        }
    }

    // The game's display setup reads the Windowed console variable itself: without it, it makes the window a
    // desktop-sized popup and clips the cursor to it. It follows the window mode instead.
    inline void InstallClient(HMODULE gameClient)
    {
        auto pattern = hook::pattern(gameClient, "33 F6 8D 44 24 08 50 FF 15 ? ? ? ? 8B 3D ? ? ? ? 50 FF D7 85 F6");
        ClientSelectionHook = safetyhook::create_mid(pattern.get_first(2), [](SafetyHookContext& context)
        {
            context.esi = Mode == 0;
        });
    }
}
