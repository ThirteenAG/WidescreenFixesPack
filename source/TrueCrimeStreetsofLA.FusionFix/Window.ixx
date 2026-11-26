module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"


export module Window;

import ComVars;

export namespace TrueCrimeWindow
{
    
    inline std::atomic<HWND> Window{ nullptr };
    inline SafetyHookInline ConfigHook;
    inline decltype(&CreateWindowExA) NativeCreateWindow{};
    
    inline int Mode{}, Width{}, Height{};
    inline bool PausedByFix = false, ReleasedByFix = false;

    inline std::atomic_bool PauseOnFocusLoss{ true };
    inline std::mutex CursorOptions;
    inline bool ConfineCursor = true, CursorOverlayBlocked = false;
    inline bool (*Message)(HWND, UINT, WPARAM, LPARAM, LRESULT&){};
    inline LPARAM (*MapMouse)(LPARAM){};
    inline void (*ReleaseInput)(){};
    inline void (*BeforeForcedExit)(){};

    inline void SetCursorOverlayBlocked(bool blocked)
    {
        const std::lock_guard lock(CursorOptions);
        CursorOverlayBlocked = blocked;
        CursorConfinement::SetEnabled(ConfineCursor && !CursorOverlayBlocked);
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

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, WindowProc, id);
            auto active = window;
            if (Window.compare_exchange_strong(active, nullptr) && ReleaseInput)
                ReleaseInput();
            return DefSubclassProc(window, message, wParam, lParam);
        }
        if (window != Window.load())
            return DefSubclassProc(window, message, wParam, lParam);

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
        // The game never quits on WM_CLOSE. In its menus the main loop ends the way Exit does; during play the
        // game's own teardown of a running mission isn't reliable, so the process ends once the window and its
        // device are released.
        if (message == WM_CLOSE && Controller.is_initialized() && Controller.get())
        {
            if (GameMode.is_initialized() && GameMode.get() < 2)
            {
                DestroyWindow(window);
                if (BeforeForcedExit)
                    BeforeForcedExit();
                TerminateProcess(GetCurrentProcess(), 0);
            }
            *reinterpret_cast<int*>(Controller.get() + 0x370) = 1;
            return 0;
        }
        if (message == WM_MOUSEMOVE && MapMouse)
            lParam = MapMouse(lParam);
        if (message == WM_ACTIVATEAPP && EngineState.is_initialized())
        {
            const int state = EngineState.get();
            // Fullscreen loses the device with the focus: the game's own way is taken, the loop releases the device
            // and idles in state 0 and creates it again in state 2.
            if (Mode == 0)
            {
                if (!wParam && state == 3)
                {
                    EngineState = 0;
                    ReleasedByFix = true;
                }
                else if (wParam && ReleasedByFix)
                {
                    // Not released yet: back once the loop is idle
                    if (state == 0)
                        PostMessageW(window, WM_ACTIVATEAPP, TRUE, 0);
                    else
                    {
                        if (state == 1)
                            EngineState = 2;
                        ReleasedByFix = false;
                    }
                }
                return 0;
            }
            // In a window the device stays; the loop's idle state (1) stops the game the same way
            const bool pause = !wParam && PauseOnFocusLoss.load(std::memory_order_relaxed);
            if (pause && state == 3)
            {
                EngineState = 1;
                PausedByFix = true;
            }
            else if (!pause && PausedByFix)
            {
                if (state == 1)
                    EngineState = 3;
                PausedByFix = false;
            }
            return 0;
        }

        if (message == WM_GETMINMAXINFO && Mode)
        {
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
            auto& limits = *reinterpret_cast<MINMAXINFO*>(lParam);
            limits.ptMaxSize.x = std::max(limits.ptMaxSize.x, rectangle.right - rectangle.left);
            limits.ptMaxSize.y = std::max(limits.ptMaxSize.y, rectangle.bottom - rectangle.top);
            limits.ptMaxTrackSize.x = std::max(limits.ptMaxTrackSize.x, rectangle.right - rectangle.left);
            limits.ptMaxTrackSize.y = std::max(limits.ptMaxTrackSize.y, rectangle.bottom - rectangle.top);
            return result;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline HWND WINAPI CreateEngineWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style,
        int x, int y, int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const bool game = name && reinterpret_cast<uintptr_t>(name) > 0xFFFF &&
            std::strcmp(name, "TrueCrime_PC") == 0;
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
            if (!SetWindowSubclass(result, WindowProc, 1, 0))
            {
                OutputDebugStringA("True Crime Fusion Fix: window message hook unavailable.\n");
                return result;
            }
            Window.store(result);
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

    // A new resolution: the window gets the new client size
    inline void Resize(int width, int height)
    {
        Width = width;
        Height = height;
        const auto window = Window.load();
        if (!window || Mode != 1)
            return;
        const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)), DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
        SetWindowPos(window, nullptr, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
            rectangle.bottom - rectangle.top, SWP_NOACTIVATE | SWP_NOZORDER);
    }

    inline UINT __cdecl ReadConfig(const char* section, const char* key, int fallback, char registry)
    {
        if (section && key && !_stricmp(section, "Renderer") && !_stricmp(key, "Windowed"))
            return Mode != 0;
        return ConfigHook.ccall<UINT>(section, key, fallback, registry);
    }

    inline void Init(int width, int height)
    {
        Width = width;
        Height = height;
        const auto refreshPause = []()
        {
            static std::mutex mutex;
            const std::lock_guard lock(mutex);
            PauseOnFocusLoss.store(FixSettings().Get("MAIN.PauseOnFocusLoss") == "1", std::memory_order_relaxed);
            if (const auto window = Window.load())
                PostMessageW(window, WM_ACTIVATEAPP, GetForegroundWindow() == window, 0);
        };
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", refreshPause);
        refreshPause();
        auto loop = hook::pattern("55 8B EC 83 E4 F0 83 EC 64 A1 ? ? ? ? 8B 88 70 03 00 00 53 33 DB 3B CB 56 57 0F 85 ? ? ? ? E8 ? ? ? ? A1");
        EngineState.SetAddress(*loop.get_first<int*>(39));
        Controller.SetAddress(*loop.get_first<uintptr_t*>(10));
        auto config = hook::pattern("8A 44 24 10 81 EC A8 00 00 00 84 C0 53 8B 9C 24 B0 00 00 00 56 57");
        if (config.empty()) // TrueCrimeMB
            config = hook::pattern("81 EC AC 00 00 00 A1 ? ? ? ? 53 8B 9C 24 B4 00 00 00 89 84 24 AC 00 00 00");
        ConfigHook = safetyhook::create_inline(config.get_first(), ReadConfig);
        const auto refreshCursor = []()
        {
            const std::lock_guard lock(CursorOptions);
            ConfineCursor = FixSettings().Get("MAIN.ConfineCursor") == "1";
            CursorConfinement::SetEnabled(ConfineCursor && !CursorOverlayBlocked);
        };
        FixSettings().OnChange("MAIN.ConfineCursor", refreshCursor);
        refreshCursor();
        auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("CreateWindowExA", CreateEngineWindow));
        NativeCreateWindow = reinterpret_cast<decltype(NativeCreateWindow)>(imports.at("CreateWindowExA").get());
    }
}
