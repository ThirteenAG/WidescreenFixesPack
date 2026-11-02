module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/WindowCursor.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Window;

import ComVars;
import Overlay;
import RawInput;

// Window modes (fullscreen, framed window, borderless), DPI-correct placement and focus handling. The game keeps its
// own resolution choice (startup dialog, prefs.dat); the fix decides only whether it runs in a window.
namespace TCWindow
{
    int Mode = 0;
    HWND Window = nullptr;
    std::atomic_bool PauseOnFocusLoss{ true };
    SafetyHookInline ValidateHook, ProcedureHook;
    void (__cdecl* EnumerateModes)() = nullptr;
    decltype(&CreateWindowExA) NativeCreateWindow = nullptr;

    RECT Monitor(HWND window)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor))
            return { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
        return monitor.rcMonitor;
    }

    // Window rectangle for a client area of the game's size: centered on its monitor; a client as big as the monitor
    // covers it exactly with the frame outside the screen
    RECT Placement(HWND window, DWORD style, DWORD extended)
    {
        MONITORINFO monitor{ sizeof(monitor) };
        if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitor))
        {
            monitor.rcMonitor = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
            monitor.rcWork = monitor.rcMonitor;
        }
        if (Mode == 2)
            return monitor.rcMonitor;
        const int width = nResolutionWidth, height = nResolutionHeight;
        RECT rectangle{ 0, 0, width, height };
        using Adjust = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using Dpi = UINT(WINAPI*)(HWND);
        const auto user = GetModuleHandleW(L"user32.dll");
        const auto adjust = reinterpret_cast<Adjust>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
        const auto dpi = reinterpret_cast<Dpi>(GetProcAddress(user, "GetDpiForWindow"));
        if (!adjust || !adjust(&rectangle, style, FALSE, extended, window && dpi ? dpi(window) : 96))
            AdjustWindowRectEx(&rectangle, style, FALSE, extended);
        const auto& area = width >= monitor.rcMonitor.right - monitor.rcMonitor.left
            || height >= monitor.rcMonitor.bottom - monitor.rcMonitor.top ? monitor.rcMonitor : monitor.rcWork;
        OffsetRect(&rectangle, area.left + (area.right - area.left - width) / 2, area.top + (area.bottom - area.top - height) / 2);
        return rectangle;
    }

    void Corners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto setAttribute = []() -> SetAttribute
        {
            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const DWORD preference = 1; // DWMWCP_DONOTROUND
        if (setAttribute)
            setAttribute(window, 33, &preference, sizeof(preference)); // DWMWA_WINDOW_CORNER_PREFERENCE
    }

    void Place(HWND window)
    {
        const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
            DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
        SetWindowPos(window, nullptr, rectangle.left, rectangle.top, rectangle.right - rectangle.left,
            rectangle.bottom - rectangle.top, SWP_NOACTIVATE | SWP_NOZORDER);
    }

    // Called first by the game's window procedure; true when the message is handled here
    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if (message == WM_ACTIVATE)
        {
            const bool active = LOWORD(wParam) != WA_INACTIVE && !HIWORD(wParam);
            const bool pause = PauseOnFocusLoss.load(std::memory_order_relaxed);
            if (Mode == 0)
            {
                // The game's own handler minimizes the fullscreen window and loses the renderer: only its pause flag
                if (!active)
                {
                    bAppInactive = pause;
                    result = DefWindowProcA(window, message, wParam, lParam);
                    return true;
                }
                bAppInactive = false;
            }
            else if (!active && !pause)
            {
                // The game keeps running (and its sound playing) in the background
                result = DefWindowProcA(window, message, wParam, lParam);
                return true;
            }
            return false;
        }
        if (Mode == 0)
        {
            if ((message == WM_ACTIVATEAPP && !wParam) || ((message == WM_NCACTIVATE || message == WM_KILLFOCUS ||
                message == WM_SHOWWINDOW) && !wParam) || (message == WM_SIZE && wParam == SIZE_MINIMIZED) ||
                (message == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_MINIMIZE) ||
                (message == WM_WINDOWPOSCHANGED && lParam && (reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_HIDEWINDOW)))
            {
                result = DefWindowProcA(window, message, wParam, lParam);
                return true;
            }
            return false;
        }
        if (message == 0x02E0) // WM_DPICHANGED
        {
            Place(window);
            result = 0;
            return true;
        }
        if (message == WM_WINDOWPOSCHANGING && !IsIconic(window))
        {
            auto& position = *reinterpret_cast<WINDOWPOS*>(lParam);
            if (!(position.flags & SWP_NOSIZE))
            {
                // The game asks for one pixel more than its client size; the client keeps the back buffer's size
                const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                    DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
                position.x = rectangle.left;
                position.y = rectangle.top;
                position.cx = rectangle.right - rectangle.left;
                position.cy = rectangle.bottom - rectangle.top;
                position.flags &= ~SWP_NOMOVE;
            }
            return false;
        }
        if (message == WM_GETMINMAXINFO)
        {
            const auto rectangle = Placement(window, DWORD(GetWindowLongPtrW(window, GWL_STYLE)),
                DWORD(GetWindowLongPtrW(window, GWL_EXSTYLE)));
            auto& limits = *reinterpret_cast<MINMAXINFO*>(lParam);
            limits.ptMaxSize.x = std::max(limits.ptMaxSize.x, rectangle.right - rectangle.left);
            limits.ptMaxSize.y = std::max(limits.ptMaxSize.y, rectangle.bottom - rectangle.top);
            limits.ptMaxTrackSize.x = std::max(limits.ptMaxTrackSize.x, rectangle.right - rectangle.left);
            limits.ptMaxTrackSize.y = std::max(limits.ptMaxTrackSize.y, rectangle.bottom - rectangle.top);
            result = 0;
            return true;
        }
        return false;
    }

    // The game's window procedure (its class procedure, so every message passes here first)
    LRESULT __fastcall Procedure(void* self, void* edx, HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;
        if (TCOverlay::Message(window, message, wParam, lParam, result))
            return result;
        if (message == WM_DESTROY)
            RWGTA::UpdateCheck::OnExit();
        if (Message(window, message, wParam, lParam, result))
            return result;
        RawInputWindow(window);
        return ProcedureHook.unsafe_fastcall<LRESULT>(self, edx, window, message, wParam, lParam);
    }

    HWND WINAPI CreateGameWindow(DWORD extended, LPCSTR name, LPCSTR title, DWORD style, int x, int y,
        int width, int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID parameter)
    {
        const bool game = !Window && !parent;
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
        const auto result = NativeCreateWindow(extended, name, title, style, x, y, width, height, parent, menu,
            instance, parameter);
        if (game && result)
        {
            Window = result;
            Corners(result);
            WindowCursor::Attach(result);
        }
        return result;
    }

    // The game's startup check of its resolution against the display modes, with its settings dialog (also opened
    // with Ctrl held). A window takes any size; fullscreen keeps the game's own check.
    void __cdecl Validate()
    {
        if (Mode == 0 || (GetAsyncKeyState(VK_CONTROL) & 0x8000) || bShowSettingsDialog || nResolutionWidth < 320 || nResolutionHeight < 240)
            ValidateHook.unsafe_ccall();
        else
            EnumerateModes();
        bWindowed = Mode != 0;
        if (Mode == 2)
        {
            const auto monitor = Monitor(nullptr);
            nResolutionWidth = monitor.right - monitor.left;
            nResolutionHeight = monitor.bottom - monitor.top;
        }
    }

    void Install()
    {
        auto& settings = FixSettings();
        Mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        PauseOnFocusLoss.store(settings.Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        settings.OnChange("MAIN.PauseOnFocusLoss", []()
        {
            PauseOnFocusLoss.store(FixSettings().Bool("MAIN.PauseOnFocusLoss"), std::memory_order_relaxed);
        });

        // The settings' defaults: version, size, depth, refresh, dialog, windowed
        auto defaults = hook::pattern("BE 24 01 00 00 89 35 ? ? ? ? C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00");
        nResolutionWidth.SetAddress(*defaults.get_first<int*>(13));
        nResolutionHeight.SetAddress(*defaults.get_first<int*>(23));
        bShowSettingsDialog.SetAddress(*defaults.get_first<int*>(52));
        bWindowed.SetAddress(*defaults.get_first<int*>(58));
        bAppInactive.SetAddress(*hook::pattern("66 83 BC 24 ? ? ? ? 00 0F 94 C2 88 15").get_first<bool*>(14));

        auto validate = hook::pattern("83 EC 0C 53 56 57 E8 ? ? ? ? 6A 11 FF 15");
        EnumerateModes = reinterpret_cast<decltype(EnumerateModes)>(injector::GetBranchDestination(validate.get_first(6)).as_int());
        ValidateHook = safetyhook::create_inline(validate.get_first(), Validate);

        auto procedure = find_pattern("81 EC 90 00 00 00 53", "83 EC 48 53 8B 5C 24");
        ProcedureHook = safetyhook::create_inline(procedure.get_first(), Procedure);

        WindowCursor::Initialize(settings);
        auto imports = IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll",
            std::forward_as_tuple("CreateWindowExA", CreateGameWindow));
        NativeCreateWindow = reinterpret_cast<decltype(NativeCreateWindow)>(imports.at("CreateWindowExA").get());
    }
}

class Window
{
public:
    Window()
    {
        WFP::onInitEvent() += []()
        {
            TCWindow::Install();
        };
    }
} Window;
