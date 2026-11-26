module;

#include "stdafx.h"
#include "FixSettings.hpp"

export module Window;

import Build;
import Settings;
import RawMouse;

namespace
{
    std::atomic<HWND> gameWindow = nullptr;
    std::atomic<bool> captureCursor = false;
    SafetyHookInline wndProcHook;
    std::mutex cursorMutex;
    bool ownsCursorClip = false;
    RECT lastCursorClip{};

    // Unreal reads every WM_INPUT on its own and runs Slate's mouse move handling for it; a high polling rate mouse
    // sends thousands a second and the pump never catches up. A WM_INPUT reads every report still queued at once
    // (GetRawInputBuffer removes their messages). Unreal receives each batch while its raw handle is valid.
    // Window procedure thread only.
    constexpr LONGLONG DeliverIntervalUs = 4000;
    constexpr UINT_PTR LegacyMoveTimer = 0x46464D56;
    SafetyHookMid rawReadHook;
    uintptr_t rawReadReturn = 0;
    struct MouseDelivery
    {
        const RAWINPUT& input;
        HRAWINPUT handle;
    };
    thread_local const MouseDelivery* delivering = nullptr;
    WORD heldButtons = 0;
    LONGLONG lastDelivery = 0;
    bool legacyMovePending = false;
    bool legacyOff = false; // as of the last hand-over
    LONGLONG lastLegacyCheck = 0;

    LONGLONG NowUs()
    {
        static const LONGLONG frequency = []
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            return f.QuadPart;
        }();
        LARGE_INTEGER t;
        QueryPerformanceCounter(&t);
        return t.QuadPart / frequency * 1000000 + t.QuadPart % frequency * 1000000 / frequency;
    }

    struct ButtonReport
    {
        USHORT flags;
        USHORT data;
    };

    LONG ClampDelta(int64_t value)
    {
        return static_cast<LONG>(std::clamp<int64_t>(value, LONG_MIN, LONG_MAX));
    }

    template<typename Visit>
    void ReadQueuedMouse(Visit&& visit)
    {
        alignas(8) uint8_t buffer[16384];
        for (;;)
        {
            UINT size = sizeof(buffer);
            const auto count = GetRawInputBuffer(reinterpret_cast<RAWINPUT*>(buffer), &size, sizeof(RAWINPUTHEADER));
            if (!count || count == UINT(-1))
                return;
            auto next = reinterpret_cast<uintptr_t>(buffer);
            for (UINT i = 0; i < count; ++i)
            {
                const auto input = reinterpret_cast<const RAWINPUT*>(next);
                next = (next + input->header.dwSize + 7) & ~uintptr_t(7);
                if (input->header.dwType == RIM_TYPEMOUSE)
                    visit(*input);
            }
        }
    }

    bool WantLegacyMouse(HWND window)
    {
        if (GetForegroundWindow() != window || IsIconic(window))
            return true;
        // The game's own mouse camera follows the hidden cursor, which it centres every frame: it needs every
        // move (the capture spares the hit tests, see UpdateMouseCapture)
        CURSORINFO shown{ sizeof(shown) };
        if (captureCursor && !(Settings.rawMouseInput && IsRawMouseReady()) && GetCursorInfo(&shown)
            && !(shown.flags & CURSOR_SHOWING))
            return true;
        GUITHREADINFO info{ sizeof(info) };
        if (!GetGUIThreadInfo(GetCurrentThreadId(), &info)
            || (info.flags & (GUI_INMENUMODE | GUI_INMOVESIZE | GUI_POPUPMENUMODE | GUI_SYSTEMMENUMODE))
            || (info.hwndCapture && info.hwndCapture != window))
            return true;
        POINT cursor{};
        RECT client{};
        return !GetCursorPos(&cursor) || !ScreenToClient(window, &cursor) || !GetClientRect(window, &client)
            || !PtInRect(&client, cursor);
    }

    // Without capture Windows hit-tests every report of the mouse and sends WM_NCHITTEST and WM_SETCURSOR for it
    // (Unreal asks Slate each time). While the game's own mouse camera has the hidden cursor, the window holds the
    // capture; the moves still come as WM_MOUSEMOVE, merged in the queue.
    bool heldCapture = false;

    void UpdateMouseCapture(HWND window)
    {
        CURSORINFO cursor{ sizeof(cursor) };
        const bool want = captureCursor && !(Settings.rawMouseInput && IsRawMouseReady()) && GetForegroundWindow() == window
            && GetCursorInfo(&cursor) && !(cursor.flags & CURSOR_SHOWING);
        if (want)
        {
            if (GetCapture() != window)
                SetCapture(window);
            heldCapture = true;
        }
        else if (heldCapture)
        {
            heldCapture = false;
            if (GetCapture() == window)
                ReleaseCapture();
        }
    }

    // Without legacy mouse messages (see SetLegacyMouse) Unreal gets one WM_SETCURSOR and WM_MOUSEMOVE per read
    // instead of one per report, and the button and wheel changes, through the whole window procedure chain
    void PassLegacyMouse(HWND window, bool moved, const std::vector<ButtonReport>& reports)
    {
        struct Button
        {
            USHORT down;
            USHORT up;
            WORD held;
            UINT downMessage;
            UINT upMessage;
            WORD xButton;
        };
        static constexpr Button buttons[] =
        {
            { RI_MOUSE_LEFT_BUTTON_DOWN, RI_MOUSE_LEFT_BUTTON_UP, MK_LBUTTON, WM_LBUTTONDOWN, WM_LBUTTONUP, 0 },
            { RI_MOUSE_RIGHT_BUTTON_DOWN, RI_MOUSE_RIGHT_BUTTON_UP, MK_RBUTTON, WM_RBUTTONDOWN, WM_RBUTTONUP, 0 },
            { RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, MK_MBUTTON, WM_MBUTTONDOWN, WM_MBUTTONUP, 0 },
            { RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, MK_XBUTTON1, WM_XBUTTONDOWN, WM_XBUTTONUP, XBUTTON1 },
            { RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP, MK_XBUTTON2, WM_XBUTTONDOWN, WM_XBUTTONUP, XBUTTON2 },
        };
        auto& held = heldButtons;
        const auto proc = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));

        POINT cursor{};
        GetCursorPos(&cursor);
        const auto screen = MAKELPARAM(cursor.x, cursor.y);
        ScreenToClient(window, &cursor);
        const auto client = MAKELPARAM(cursor.x, cursor.y);
        if (moved)
        {
            CallWindowProcW(proc, window, WM_SETCURSOR, reinterpret_cast<WPARAM>(window), MAKELPARAM(HTCLIENT, WM_MOUSEMOVE));
            CallWindowProcW(proc, window, WM_MOUSEMOVE, held, client);
        }
        for (const auto& report : reports)
        {
            for (const auto& button : buttons)
            {
                if (report.flags & button.down)
                {
                    held |= button.held;
                    CallWindowProcW(proc, window, button.downMessage, MAKEWPARAM(held, button.xButton), client);
                }
                if (report.flags & button.up)
                {
                    held &= ~button.held;
                    CallWindowProcW(proc, window, button.upMessage, MAKEWPARAM(held, button.xButton), client);
                }
            }
            if (report.flags & RI_MOUSE_WHEEL)
                CallWindowProcW(proc, window, WM_MOUSEWHEEL, MAKEWPARAM(held, report.data), screen);
            if (report.flags & RI_MOUSE_HWHEEL)
                CallWindowProcW(proc, window, WM_MOUSEHWHEEL, MAKEWPARAM(held, report.data), screen);
        }
    }
}

namespace
{
    // The last movement held back passes on once the queue is empty
    void CALLBACK PassLegacyMove(HWND window, UINT, UINT_PTR timer, DWORD)
    {
        KillTimer(window, timer);
        static const std::vector<ButtonReport> none;
        if (legacyMovePending && SetLegacyMouse(WantLegacyMouse(window), window))
            PassLegacyMouse(window, true, none);
        legacyMovePending = false;
    }

    void ResetMouseDelivery(HWND window)
    {
        KillTimer(window, LegacyMoveTimer);
        legacyMovePending = false;
        heldButtons = 0;
        lastDelivery = 0;
    }
}

export bool IsGameFocused()
{
    const auto window = gameWindow.load();
    return window && window == GetForegroundWindow();
}

export void LockCursor()
{
    std::scoped_lock lock(cursorMutex);
    const auto window = gameWindow.load();
    if (!FixSettings().Bool("MAIN.ConfineCursor") || !captureCursor.load() || !window || window != GetForegroundWindow())
    {
        // An inactive game must not release another application's capture.
        if (ownsCursorClip)
        {
            RECT current{};
            if (GetClipCursor(&current) && EqualRect(&current, &lastCursorClip))
                ClipCursor(nullptr);
            ownsCursorClip = false;
        }
        return;
    }
    RECT rect{};
    if (GetClientRect(window, &rect) && rect.right > 2 && rect.bottom > 2)
    {
        MapWindowPoints(window, nullptr, reinterpret_cast<POINT*>(&rect), 2);
        ++rect.left;
        ++rect.top;
        --rect.right;
        --rect.bottom;
        if (ClipCursor(&rect))
        {
            lastCursorClip = rect;
            ownsCursorClip = true;
        }
    }
}

export void SetCursorCapture(bool capture)
{
    captureCursor = capture;
    LockCursor();
}

LRESULT CustomWndProc(void* application, HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (window == GetForegroundWindow())
        gameWindow = window;
    // A minimized window reports a 0x0 client area. The game then frees its
    // scene targets while frames keep rendering and crashes on restore.
    // Keep the last size; the restore message brings back the same size.
    if (message == WM_SIZE && wParam == SIZE_MINIMIZED)
        return 0;
    if (message == WM_INPUT)
    {
        auto generation = MouseReadGeneration();
        RAWINPUT input{};
        UINT size = sizeof(input);
        const auto read = GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
        if (read != UINT(-1) && read >= sizeof(RAWINPUTHEADER) + sizeof(RAWMOUSE) && input.header.dwType == RIM_TYPEMOUSE)
        {
            int64_t x = input.data.mouse.lLastX;
            int64_t y = input.data.mouse.lLastY;
            bool moved = x != 0 || y != 0;
            std::vector<ButtonReport> buttons;
            const auto rememberButtons = [&](const RAWMOUSE& mouse)
            {
                if (mouse.usButtonFlags)
                    buttons.push_back({ mouse.usButtonFlags, mouse.usButtonData });
            };
            rememberButtons(input.data.mouse);
            const auto deliver = [&]()
            {
                input.data.mouse.lLastX = ClampDelta(x);
                input.data.mouse.lLastY = ClampDelta(y);
                if (Settings.rawMouseInput)
                    ReadRawMouse(window, input.header.wParam, input.data.mouse, generation);
                const MouseDelivery delivery{ input, reinterpret_cast<HRAWINPUT>(lParam) };
                const auto previous = std::exchange(delivering, &delivery);
                const auto result = wndProcHook.ccall<LRESULT>(application, window, message, input.header.wParam, lParam);
                delivering = previous;
                return result;
            };
            if (IsOnlyRawMouse())
            {
                ReadQueuedMouse([&](const RAWINPUT& next)
                {
                    const auto& mouse = next.data.mouse;
                    if ((input.data.mouse.usFlags | mouse.usFlags) & MOUSE_MOVE_ABSOLUTE ||
                        next.header.hDevice != input.header.hDevice || next.header.wParam != input.header.wParam)
                    {
                        deliver();
                        input = next;
                        x = 0;
                        y = 0;
                    }
                    x += mouse.lLastX;
                    y += mouse.lLastY;
                    moved |= mouse.lLastX != 0 || mouse.lLastY != 0;
                    rememberButtons(mouse);
                });
                MessagePumpGuard::EndPassAtRawInput();
            }
            const auto result = deliver();
            const auto now = NowUs();
            if (buttons.empty() && now - lastDelivery < DeliverIntervalUs)
            {
                if (legacyOff && moved && !legacyMovePending)
                {
                    legacyMovePending = true;
                    SetTimer(window, LegacyMoveTimer, 4, PassLegacyMove);
                }
                return result;
            }
            lastDelivery = now;
            legacyOff = SetLegacyMouse(WantLegacyMouse(window), window);
            if (legacyOff)
                PassLegacyMouse(window, moved || legacyMovePending, buttons);
            legacyMovePending = false;
            return result;
        }
    }
    if (message == WM_MOUSEMOVE)
    {
        UpdateMouseCapture(window);
        // Legacy moves come while the mouse has no RIDEV_NOLEGACY registration yet
        const auto now = NowUs();
        if (now - lastLegacyCheck >= DeliverIntervalUs)
        {
            lastLegacyCheck = now;
            if (!WantLegacyMouse(window))
            {
                if (!legacyOff)
                    heldButtons = LOWORD(wParam) & (MK_LBUTTON | MK_RBUTTON | MK_MBUTTON | MK_XBUTTON1 | MK_XBUTTON2);
                legacyOff = SetLegacyMouse(false, window);
            }
        }
    }
    if (message == WM_CAPTURECHANGED && reinterpret_cast<HWND>(lParam) != window)
        ResetMouseDelivery(window);
    if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam)
        || message == WM_CLOSE || message == WM_DESTROY)
    {
        ResetRawMouse();
        ResetMouseDelivery(window);
        legacyOff = false;
        SetLegacyMouse(true);
        if (std::exchange(heldCapture, false) && GetCapture() == window)
            ReleaseCapture();
        SetCursorCapture(false);
        WFP::onActivateApp().executeAll(false);
    }
    else if (message == WM_SETFOCUS || (message == WM_ACTIVATEAPP && wParam))
    {
        ResetRawMouse();
        WFP::onActivateApp().executeAll(true);
    }
    const auto result = wndProcHook.ccall<LRESULT>(application, window, message, wParam, lParam);
    if (message == WM_SIZE || message == WM_MOVE)
        LockCursor();
    return result;
}

class WindowModule
{
public:
    WindowModule()
    {
        FixSettings().OnChange("MAIN.ConfineCursor", []() { LockCursor(); });
        WFP::onInitEvent() += []()
        {
            wndProcHook = safetyhook::create_inline(WindowProcedureAddress(), CustomWndProc);
            MessagePumpGuard::Install(GetModuleHandleW(nullptr));
            // Use the assembled report for this handle without reading its first packet again.
            auto* rawRead = RawInputReadAddress();
            rawReadReturn = reinterpret_cast<uintptr_t>(rawRead) + 6;
            rawReadHook = safetyhook::create_mid(rawRead, [](SafetyHookContext& context)
            {
                if (!delivering || reinterpret_cast<HRAWINPUT>(context.rcx) != delivering->handle ||
                    static_cast<UINT>(context.rdx) != RID_INPUT || !context.r8 || !context.r9)
                    return;

                auto& size = *reinterpret_cast<UINT*>(context.r9);
                if (size < sizeof(RAWINPUT))
                    return;

                std::memcpy(reinterpret_cast<void*>(context.r8), &delivering->input, sizeof(RAWINPUT));
                size = sizeof(RAWINPUT);
                context.rax = sizeof(RAWINPUT);
                context.rip = rawReadReturn;
            });
        };
        WFP::onGameInitEvent() += []()
        {
            ResetRawMouse();
            SetCursorCapture(false);
        };
        WFP::onShutdownEvent() += []()
        {
            SetCursorCapture(false);
            ShutdownRawMouse();
        };
    }
} WindowModuleInstance;
