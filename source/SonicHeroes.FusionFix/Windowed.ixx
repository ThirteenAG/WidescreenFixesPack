module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

export module Windowed;

import ComVars;
import Resolution;

// The game has a window mode of its own (Screen_Full 0 in sonic_h.ini): a captioned window at the render size that
// it keeps at the top-left corner of the desktop. The fix sets that flag from MAIN.WindowMode and routes the game's
// window calls through the shared windowed mode: centred, framed (optionally resizable) or borderless. Only a
// resizable window renders at its client size; every other mode renders at the chosen size, and a borderless window
// fitted to the monitor shows that picture scaled at its own aspect ratio.
export namespace Windowed
{
    enum class Mode
    {
        Fullscreen,
        Framed,
        Borderless,
    };

    inline Mode WindowMode = Mode::Fullscreen;
    inline bool Resizable = false;
    inline HWND Window = nullptr;

    // The window procedure sees the client size the game renders at; a resizable window resizes the picture
    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
    {
        switch (message)
        {
            case WM_SIZE:
                if (wParam == SIZE_MINIMIZED)
                    break;
                if (Resizable)
                    Resolution::Apply(LOWORD(lParam), HIWORD(lParam));
                else
                    lParam = MAKELPARAM(Screen.Width, Screen.Height);
                break;
            case WM_NCDESTROY:
                RemoveWindowSubclass(window, Procedure, 1);
                break;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline HWND WINAPI CreateWindowExHook(DWORD exStyle, LPCSTR className, LPCSTR windowName, DWORD style, int x, int y, int width,
        int height, HWND parent, HMENU menu, HINSTANCE instance, LPVOID param)
    {
        const HWND created = WindowMode == Mode::Fullscreen ?
            CreateWindowExA(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param) :
            WindowedModeWrapper::CreateWindowExA_Hook(exStyle, className, windowName, style, x, y, width, height, parent, menu, instance, param);
        if (created && !parent && !Window)
        {
            Window = created;
            SetWindowSubclass(created, Procedure, 1, 0);
        }
        return created;
    }

    // The game's own windowed path puts its window in the desktop's corner (at start and when the display changes);
    // a framed window is centred at its size instead. Resizing it with the mouse keeps going through.
    inline BOOL WINAPI SetWindowPosHook(HWND window, HWND after, int x, int y, int width, int height, UINT flags)
    {
        if (window == Window && WindowMode == Mode::Framed && !(flags & SWP_NOMOVE))
        {
            WindowedModeWrapper::afterCreateWindow();
            return TRUE;
        }
        return WindowedModeWrapper::SetWindowPos_Hook(window, after, x, y, width, height, flags);
    }

    // The render size, except for a resizable window, which renders at whatever size it has
    inline BOOL WINAPI GetClientRectHook(HWND window, LPRECT rect)
    {
        if (window != Window || Resizable || !rect)
            return GetClientRect(window, rect);
        *rect = { 0, 0, Screen.Width, Screen.Height };
        return TRUE;
    }

    // In a window the game doesn't move the cursor while it is outside the game's picture
    inline BOOL WINAPI SetCursorPosHook(int x, int y)
    {
        if (WindowMode != Mode::Fullscreen && Window)
        {
            POINT cursor{};
            RECT client{};
            GetCursorPos(&cursor);
            GetClientRect(Window, &client);
            MapWindowPoints(Window, nullptr, reinterpret_cast<POINT*>(&client), 2);
            if (!PtInRect(&client, cursor) || GetForegroundWindow() != Window)
                return TRUE;
        }
        return SetCursorPos(x, y);
    }

    // The game registers its window class without the icon its executable carries
    inline ATOM WINAPI RegisterClassExHook(const WNDCLASSEXA* windowClass)
    {
        WNDCLASSEXA copy = *windowClass;
        const HINSTANCE module = GetModuleHandleW(nullptr);
        if (!copy.hIcon)
            copy.hIcon = LoadIconA(module, MAKEINTRESOURCEA(138));
        if (!copy.hIconSm)
            copy.hIconSm = static_cast<HICON>(LoadImageA(module, MAKEINTRESOURCEA(138), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
        return RegisterClassExA(&copy);
    }

    // The quit question opens in the middle of the game's monitor, in front of the game
    inline BOOL WINAPI CenterDialog(HWND dialog)
    {
        RECT box{};
        GetWindowRect(dialog, &box);
        MONITORINFO monitor{ sizeof(monitor) };
        GetMonitorInfoW(MonitorFromWindow(Window ? Window : dialog, MONITOR_DEFAULTTONEAREST), &monitor);
        const RECT& area = monitor.rcWork;
        const int width = box.right - box.left;
        const int height = box.bottom - box.top;
        SetWindowPos(dialog, HWND_TOP, area.left + (area.right - area.left - width) / 2, area.top + (area.bottom - area.top - height) / 2,
            0, 0, SWP_NOSIZE);
        SetForegroundWindow(dialog);
        return UpdateWindow(dialog);
    }

    // MAIN.WindowMode 0 full screen, 1 framed window (MAIN.WindowResize: resizable), 2 borderless window
    // (MAIN.BorderlessScaling 0 the monitor's size, 1 the chosen size fitted to the monitor, 2 the chosen size)
    bool CoversMonitor()
    {
        auto& settings = FixSettings();
        return settings.Int("MAIN.WindowMode") == 2 && settings.Int("MAIN.BorderlessScaling") == 0;
    }

    void Init()
    {
        auto& settings = FixSettings();
        switch (std::clamp(settings.Int("MAIN.WindowMode"), 0, 2))
        {
            case 1:
                WindowMode = Mode::Framed;
                Resizable = settings.Bool("MAIN.WindowResize");
                break;
            case 2:
                WindowMode = Mode::Borderless;
                break;
            default:
                WindowMode = Mode::Fullscreen;
                break;
        }

        WindowedModeWrapper::EnableDpiAwareness();
        WindowedModeWrapper::bBorderlessWindowed = WindowMode == Mode::Borderless;
        WindowedModeWrapper::bEnableWindowResize = Resizable;
        WindowedModeWrapper::bScaleWindow = WindowMode == Mode::Borderless && settings.Int("MAIN.BorderlessScaling") == 1;

        // Screen_Full from sonic_h.ini, read right after Screen_Size_Selection: the fix decides it
        auto config = hook::pattern("A2 ? ? ? ? E9 ? ? ? ? 8B 15 ? ? ? ? 8D 4C 24 10 51 68 ? ? ? ? 50 FF 92 30 01 00 00 8B 4C 24 1C 33 C0 83 C4 0C 3B CD 0F 95 C0 A3");
        const auto fullscreenWrite = config.get_first<uint8_t>(0x2F);
        injector::WriteMemory<uint32_t>(*reinterpret_cast<uint32_t**>(fullscreenWrite + 1), WindowMode == Mode::Fullscreen, true);
        injector::MakeNOP(fullscreenWrite, 5, true);

        const auto module = GetModuleHandleW(nullptr);
        IATHook::Replace(module, "USER32.dll",
            std::forward_as_tuple("CreateWindowExA", CreateWindowExHook),
            std::forward_as_tuple("GetClientRect", GetClientRectHook),
            std::forward_as_tuple("RegisterClassExA", RegisterClassExHook));

        // The quit question: centred, and the game window is not minimised behind it
        auto question = hook::pattern("FF 15 ? ? ? ? 8B 54 24 14 8B 44 24 10 52 50 68 10 01 00 00");
        injector::MakeNOP(question.get_first(), 6, true);
        injector::MakeCALL(question.get_first(), CenterDialog, true);
        auto minimize = hook::pattern("6A 06 50 FF D5 A1 ? ? ? ? 3B C3 C6 05 ? ? ? ? 01");
        injector::MakeJMP(minimize.get_first(), minimize.get_first<uint8_t>(5), true);

        if (WindowMode == Mode::Fullscreen)
            return;

        IATHook::Replace(module, "USER32.dll",
            std::forward_as_tuple("AdjustWindowRect", WindowedModeWrapper::AdjustWindowRect_Hook),
            std::forward_as_tuple("SetWindowLongA", WindowedModeWrapper::SetWindowLongA_Hook),
            std::forward_as_tuple("SetWindowPos", SetWindowPosHook),
            std::forward_as_tuple("SetCursorPos", SetCursorPosHook));
        if (WindowMode == Mode::Framed)
            IATHook::Replace(module, "USER32.dll", std::forward_as_tuple("ShowCursor", WindowedModeWrapper::ShowCursor_Hook));
    }
}
