module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "include/WindowStyle.hpp"
#include "Shared/WindowCursor.hpp"

export module Window;

import Frontend;
import ComVars;

export namespace DeerWindow
{
    struct VideoMode
    {
        int width, height, depth, flags;
    };

    inline SafetyHookMid selectionHook, showHook, focusHook;
    inline SafetyHookInline procedureHook, clockHook;
    inline LONGLONG frameInterval = 0, nextFrame = 0;
    inline HANDLE frameTimer = nullptr;
    inline int mode = 0;
    inline bool ready = false, windowed = false;
    inline thread_local bool placing = false;

    inline bool FixDeviceLimit()
    {
        const auto module = LoadLibraryExW(L"d3dim700.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module)
            return false;
        auto native = hook::pattern(module, "B8 00 08 00 00 39 ? ? 0F 87 ? ? ? ? 39 ? ? 0F 87");
        auto limit = native.get_first<uint32_t>(1);
        auto targetLimit = hook::pattern(module, "B8 00 08 00 00 39 85 ? ? ? ? 0F 87 ? ? ? ? 39 85 ? ? ? ? 0F 87").get_first<uint32_t>(1);
        injector::WriteMemory<uint32_t>(limit, 16384, true);
        injector::WriteMemory<uint32_t>(targetLimit, 16384, true);
        FlushInstructionCache(GetCurrentProcess(), limit, sizeof(*limit));
        FlushInstructionCache(GetCurrentProcess(), targetLimit, sizeof(*targetLimit));
        return true;
    }

    // The launcher places its controls in 96-DPI pixels over a fixed-size picture; at high
    // display scaling it keeps the system's scaling while the game window stays DPI aware.
    inline INT_PTR WINAPI Dialog(HINSTANCE instance, LPCSTR name, HWND parent, DLGPROC procedure, LPARAM param)
    {
        using SetThreadContext = DPI_AWARENESS_CONTEXT(WINAPI*)(DPI_AWARENESS_CONTEXT);
        static const auto set = reinterpret_cast<SetThreadContext>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetThreadDpiAwarenessContext"));
        DPI_AWARENESS_CONTEXT previous = nullptr;
        if (set && !(previous = set(DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED)))
            previous = set(DPI_AWARENESS_CONTEXT_UNAWARE);
        const auto result = DialogBoxParamA(instance, name, parent, procedure, param);
        if (previous)
            set(previous);
        return result;
    }

    // The launcher lists only 16-bit modes, which current Windows doesn't always offer
    // (none left means the game can't start); list the 32-bit modes instead.
    inline void ListModes(HMODULE game)
    {
        auto depth = hook::pattern(game, "83 7C 24 20 10 75 4D 8D 4C 24 28 51 6A 00 68 43 01 00 00");
        injector::WriteMemory<uint8_t>(depth.get_first<uint8_t>(4), 32, true);
    }

    inline HWND Window()
    {
        auto context = game_context.is_initialized() ? DeerMenu::Read<uintptr_t>(reinterpret_cast<uintptr_t>(game_context.get_ptr())) : 0;
        return context ? DeerMenu::Read<HWND>(context) : nullptr;
    }

    inline bool ValidSize(int w, int h)
    {
        return w > 0 && h > 0 && w <= 16384 && h <= 16384;
    }

    inline bool Prepare()
    {
        if (!ready || mode == 0 || !game_context.is_initialized() || !game_selected.is_initialized() || !game_modes.is_initialized() || !game_modeCount.is_initialized() || !game_windowWidth.is_initialized() || !game_windowHeight.is_initialized())
            return false;
        auto context = DeerMenu::Read<uintptr_t>(reinterpret_cast<uintptr_t>(game_context.get_ptr()));
        auto selected = DeerMenu::Read<VideoMode*>(reinterpret_cast<uintptr_t>(game_selected.get_ptr()));
        auto modes = DeerMenu::Read<VideoMode*>(reinterpret_cast<uintptr_t>(game_modes.get_ptr()));
        const auto count = DeerMenu::Read<int>(reinterpret_cast<uintptr_t>(game_modeCount.get_ptr()));
        if (!context || !modes || count <= 0 || count > 65536)
            return false;
        const auto first = reinterpret_cast<uintptr_t>(modes);
        const auto chosen = reinterpret_cast<uintptr_t>(selected);
        if (chosen < first || chosen >= first + size_t(count) * sizeof(VideoMode) ||
            (chosen - first) % sizeof(VideoMode) || !ValidSize(selected->width, selected->height))
            return false;
        // RenderWare only enumerates a windowed mode for a compatible adapter.
        bool supported = false;
        for (int i = 0; i < count; ++i)
            if (!(modes[i].flags & 1) && ValidSize(modes[i].width, modes[i].height))
                supported = true;
        if (!supported)
            return false;
        int w = selected->width, h = selected->height;
        if (mode == 2)
        {
            const auto monitor = WindowStyle::MonitorRect(Window());
            w = monitor.right - monitor.left;
            h = monitor.bottom - monitor.top;
        }
        if (!ValidSize(w, h))
            return false;
        // The device and camera consume the same windowed dimensions.
        injector::WriteMemory<int>(&selected->width, w, true);
        injector::WriteMemory<int>(&selected->height, h, true);
        // The device and camera both consume this native mode flag.
        injector::WriteMemory<int>(&selected->flags, selected->flags & ~1, true);
        injector::WriteMemory<int>(reinterpret_cast<int*>(context + 8), 0, true);
        injector::WriteMemory<int>(game_windowWidth.get_ptr(), w, true);
        injector::WriteMemory<int>(game_windowHeight.get_ptr(), h, true);
        windowed = true;
        return true;
    }

    inline void UpdateFrameInterval(HWND window)
    {
        MONITORINFOEXW monitor{};
        monitor.cbSize = sizeof(monitor);
        DEVMODEW display{};
        display.dmSize = sizeof(display);
        DWORD rate = 60;
        if (GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor) &&
            EnumDisplaySettingsW(monitor.szDevice, ENUM_CURRENT_SETTINGS, &display) && display.dmDisplayFrequency > 1)
            rate = display.dmDisplayFrequency;
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        frameInterval = frequency.QuadPart / rate;
    }

    // The game clock counts whole milliseconds and expects vsync to pace frames; an unpaced window gives
    // zero-length frames that break movement, so wait for the monitor refresh as fullscreen does.
    inline int __cdecl Clock(float step, int fixedStep)
    {
        if (windowed && !fixedStep && frameInterval > 0)
        {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            if (nextFrame == 0 || now.QuadPart - nextFrame > frameInterval)
                nextFrame = now.QuadPart;
            while (now.QuadPart < nextFrame)
            {
                LARGE_INTEGER frequency;
                QueryPerformanceFrequency(&frequency);
                const LONGLONG remaining = nextFrame - now.QuadPart;
                if (frameTimer && remaining > frequency.QuadPart / 1000)
                {
                    LARGE_INTEGER due{};
                    due.QuadPart = -std::max<LONGLONG>(1, (remaining - frequency.QuadPart / 2000) * 10000000 / frequency.QuadPart);
                    if (SetWaitableTimer(frameTimer, &due, 0, nullptr, nullptr, FALSE))
                        WaitForSingleObject(frameTimer, INFINITE);
                }
                else
                    YieldProcessor();
                QueryPerformanceCounter(&now);
            }
            nextFrame += frameInterval;
        }
        return clockHook.ccall<int>(step, fixedStep);
    }

    inline void Place()
    {
        const auto window = Window();
        if (!window)
            return;
        WindowCursor::Attach(window);
        UpdateFrameInterval(window);
        WindowStyle::SquareCorners(window);
        if (!ready || !windowed || mode == 0 || !game_windowWidth.is_initialized() || !game_windowHeight.is_initialized() || !ValidSize(game_windowWidth.get(), game_windowHeight.get()))
            return;
        const int desiredWidth = game_windowWidth.get(), desiredHeight = game_windowHeight.get();
        struct Placement
        {
            bool previous = placing;
            Placement() { placing = true; }
            ~Placement() { placing = previous; }
        } placement;
        const DWORD frame = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        DWORD style = DWORD(GetWindowLongW(window, GWL_STYLE)) & ~(frame | WS_POPUP);
        style |= mode == 2 ? WS_POPUP : WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        SetWindowLongW(window, GWL_STYLE, LONG(style));
        SetWindowPos(window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        WindowStyle::Place(window, desiredWidth, desiredHeight);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (ready && windowed && mode != 0 && window == Window() && message == WM_GETMINMAXINFO && lParam)
        {
            const auto style = DWORD(GetWindowLongW(window, GWL_STYLE));
            const auto exStyle = DWORD(GetWindowLongW(window, GWL_EXSTYLE));
            const auto frame = WindowStyle::FrameOf(style, exStyle, WindowStyle::DpiOf(window));
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize = { 320 + frame.Width(), 240 + frame.Height() };
            info->ptMaxTrackSize = { 16384 + frame.Width(), 16384 + frame.Height() };
            return 0;
        }
        const auto result = procedureHook.stdcall<LRESULT>(window, message, wParam, lParam);
        if (ready && windowed && !placing && mode == 1 && window == Window() && IsWindowVisible(window) && message == WM_SIZE &&
            wParam != SIZE_MINIMIZED && game_windowWidth.is_initialized() && game_windowHeight.is_initialized())
        {
            const int w = LOWORD(lParam), h = HIWORD(lParam);
            if (ValidSize(w, h))
            {
                injector::WriteMemory<int>(game_windowWidth.get_ptr(), w, true);
                injector::WriteMemory<int>(game_windowHeight.get_ptr(), h, true);
            }
        }
        return result;
    }

    inline bool Init(int requested, HMODULE game = GetModuleHandleW(nullptr))
    {
        WindowCursor::Initialize(FixSettings());
        mode = std::clamp(requested, 0, 2);
        IATHook::Replace(game, "USER32.dll", std::forward_as_tuple("DialogBoxParamA", Dialog));
        ListModes(game);
        if (!FixDeviceLimit())
            return false;
        auto selection = hook::pattern(game, "8A 45 E8 BB 01 00 00 00 83 C4 08 84 C3 74 1A 8B 45 DC 8B 4D E0 8B 15 ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? 89 5A 08 8D 45 EC 8D 4D F8");
        auto selected = hook::pattern(game, "A1 ? ? ? ? 85 C0 74 03 33 C0 C3 85 C9 7C F9 3B 0D ? ? ? ? 7D F1 A1 ? ? ? ? 89 0D ? ? ? ? C1 E1 04 03 C8 B8 01 00 00 00 89 0D ? ? ? ? C3");
        auto show = hook::pattern(game, "A1 ? ? ? ? 8B 94 24 98 03 00 00 83 C4 04 8B 08 52 51 FF 15 ? ? ? ? 8B 15 ? ? ? ? 8B 02 50 FF 15");
        auto procedure = hook::pattern(game, "8B 44 24 08 83 EC 18 3D 04 01 00 00 56 57 0F 87 C4 02 00 00 0F 84 76 02 00 00 83 F8 24");
        auto clock = hook::pattern(game, "8B 44 24 08 83 EC 08 85 C0 0F 84 ? ? ? ? 6A 78 E8");
        auto focus = hook::pattern(game, "A1 ? ? ? ? 85 C0 74 0E 6A 00 6A 11 E8 ? ? ? ? 83 C4 08 EB 02 FF D3");
        game_context.SetAddress(*selection.get_first<uintptr_t*>(23));
        game_windowWidth.SetAddress(*selection.get_first<int*>(28));
        game_windowHeight.SetAddress(*selection.get_first<int*>(34));
        game_modeCount.SetAddress(*selected.get_first<int*>(18));
        game_modes.SetAddress(*selected.get_first<VideoMode**>(25));
        game_selected.SetAddress(*selected.get_first<VideoMode**>(47));
        selectionHook = safetyhook::create_mid(selection.get_first(41), [](SafetyHookContext&)
        {
            if (Prepare()) Place();
        });
        showHook = safetyhook::create_mid(show.get_first(), [](SafetyHookContext&) { Place(); });
        procedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);
        focusHook = safetyhook::create_mid(focus.get_first(5), [](SafetyHookContext& context)
        {
            if (!FixSettings().Bool("MAIN.PauseOnFocusLoss"))
                context.eax = 1;
        });
        if (mode != 0)
        {
            frameTimer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            clockHook = safetyhook::create_inline(clock.get_first(), Clock);
        }
        ready = true;
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []
        {
            if (const auto window = Window())
                PostMessageW(window, WM_NULL, 0, 0);
        });
        return true;
    }
}
