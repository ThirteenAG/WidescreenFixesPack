module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"

export module Window;

export namespace PunisherWindow
{
    inline SafetyHookInline CreateHook;
    inline HWND Window{};
    inline WNDPROC OriginalProc{};
    inline decltype(&CreateWindowExA) NativeCreateWindow{};
    inline thread_local bool Creating = false;
    inline int Mode{}, Width{}, Height{};
    inline bool(__cdecl* NativeActive)(){};
    inline std::atomic_bool PauseOnFocusLoss{ true };
    inline bool (*Message)(HWND, UINT, WPARAM, LPARAM, LRESULT&){};

    inline bool __cdecl RenderActive()
    {
        return !PauseOnFocusLoss.load(std::memory_order_relaxed) || NativeActive();
    }

    inline RECT Placement(HWND window, DWORD style, DWORD extended)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor))
        {
            monitor.rcMonitor = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
            monitor.rcWork = monitor.rcMonitor;
        }
        if (Mode == 2)
            return monitor.rcMonitor;
        RECT rectangle{ 0, 0, Width, Height };
        using Adjust = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using Dpi = UINT(WINAPI*)(HWND);
        const auto user = GetModuleHandleW(L"user32.dll");
        const auto adjust = reinterpret_cast<Adjust>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
        const auto dpi = reinterpret_cast<Dpi>(GetProcAddress(user, "GetDpiForWindow"));
        if (!adjust || !adjust(&rectangle, style, FALSE, extended, window && dpi ? dpi(window) : 96))
            AdjustWindowRectEx(&rectangle, style, FALSE, extended);
        const auto& area = Width >= monitor.rcMonitor.right - monitor.rcMonitor.left
            || Height >= monitor.rcMonitor.bottom - monitor.rcMonitor.top ? monitor.rcMonitor : monitor.rcWork;
        const int x = area.left + (area.right - area.left - Width) / 2;
        const int y = area.top + (area.bottom - area.top - Height) / 2;
        OffsetRect(&rectangle, x, y);
        return rectangle;
    }

    inline void Corners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto setAttribute = []() -> SetAttribute
        {
            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const DWORD preference = 1;
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT handled{};
        if (Message && Message(window, message, wParam, lParam, handled))
            return handled;
        if (message == 0x02E0 && Mode)
        {
            const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
            SetWindowPos(window, nullptr, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
                rectangle.bottom - rectangle.top, SWP_NOACTIVATE | SWP_NOZORDER);
            return 0;
        }
        if (message == WM_WINDOWPOSCHANGING && Mode && !IsIconic(window))
        {
            auto& position = *reinterpret_cast<WINDOWPOS*>(lParam);
            if (!(position.flags & SWP_NOSIZE))
            {
                const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                    DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
                position.x = rectangle.left;
                position.y = rectangle.top;
                position.cx = rectangle.right - rectangle.left;
                position.cy = rectangle.bottom - rectangle.top;
                position.flags &= ~SWP_NOMOVE;
            }
        }
        const auto native = OriginalProc;
        if (message == WM_GETMINMAXINFO && Mode)
        {
            const auto result = native ? CallWindowProcA(native, window, message, wParam, lParam) : 0;
            const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
            auto& limits = *reinterpret_cast<MINMAXINFO*>(lParam);
            limits.ptMaxSize.x = std::max(limits.ptMaxSize.x, rectangle.right - rectangle.left);
            limits.ptMaxSize.y = std::max(limits.ptMaxSize.y, rectangle.bottom - rectangle.top);
            limits.ptMaxTrackSize.x = std::max(limits.ptMaxTrackSize.x, rectangle.right - rectangle.left);
            limits.ptMaxTrackSize.y = std::max(limits.ptMaxTrackSize.y, rectangle.bottom - rectangle.top);
            return result;
        }
        if (message == WM_NCDESTROY)
        {
            Window = nullptr;
            OriginalProc = nullptr;
        }
        return native ? CallWindowProcA(native, window, message, wParam, lParam)
            : DefWindowProcA(window, message, wParam, lParam);
    }

    inline HWND WINAPI CreateEngineWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const bool game = Creating && !parent && !(style & WS_CHILD);
        if (game && Mode)
        {
            style = Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
            extended &= ~(WS_EX_TOPMOST | WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
            const auto rectangle = Placement(nullptr, style, extended);
            x = rectangle.left;
            y = rectangle.top;
            width = rectangle.right - rectangle.left;
            height = rectangle.bottom - rectangle.top;
        }
        const auto result = NativeCreateWindow(extended, name, title, style, x, y, width, height,
            parent, menu, instance, parameter);
        if (game && result)
        {
            Window = result;
            OriginalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(result, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
            if (!OriginalProc)
                std::terminate();
            Corners(result);
            WindowCursor::Attach(result);
            if (Mode)
            {
                const auto rectangle = Placement(result, style, extended);
                SetWindowPos(result, HWND_NOTOPMOST, rectangle.left, rectangle.top,
                    rectangle.right - rectangle.left, rectangle.bottom - rectangle.top,
                    SWP_NOACTIVATE | SWP_FRAMECHANGED);
            }
        }
        return result;
    }

    inline int __cdecl Create(int, int, char)
    {
        struct Scope
        {
            Scope() { Creating = true; }
            ~Scope() { Creating = false; }
        } scope;
        return CreateHook.ccall<int>(Width, Height, static_cast<char>(Mode == 0));
    }

    inline void Init(int width, int height)
    {
        PauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []()
        {
            PauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        });
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        auto focusOff = hook::pattern("E8 ? ? ? ? 8A D8 F6 DB 1A DB 6A 00").get_first();
        auto focusOn = hook::pattern("E8 ? ? ? ? 8A D8 F6 DB 1A DB 6A 01").get_first();
        auto active = hook::pattern("A0 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A0 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC A1 ? ? ? ? 8B 4C 24 04 89 0C 85");
        NativeActive = reinterpret_cast<decltype(NativeActive)>(active.get_first());
        injector::MakeCALL(focusOff, RenderActive, true);
        injector::MakeCALL(focusOn, RenderActive, true);
        auto create = hook::pattern("55 8B EC 83 E4 F8 81 EC 34 08 00 00 A1 ? ? ? ? 53 56 57");
        auto window = hook::pattern("57 56 50 51 52 68 ? ? ? ? 68 ? ? ? ? 53 FF 15 ? ? ? ? 55");
        Width = width;
        Height = height;
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        auto slot = *window.get_first<decltype(NativeCreateWindow)*>(18);
        NativeCreateWindow = *slot;
        CreateHook = safetyhook::create_inline(create.get_first(), Create);
        injector::WriteMemory<decltype(NativeCreateWindow)>(slot, CreateEngineWindow, true);
    }
}
