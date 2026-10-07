module;

#include "stdafx.h"

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
    if (!captureCursor.load() || !window || window != GetForegroundWindow())
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
    // Unreal still receives WM_INPUT and performs its own cleanup and normal
    // menu/button processing. Reading an HRAWINPUT does not consume it.
    if (Settings.rawMouseInput)
        ReadRawMouseMessage(window, message, wParam, lParam);
    if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam)
        || message == WM_CLOSE || message == WM_DESTROY)
    {
        ResetRawMouse();
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
        WFP::onInitEvent() += []()
        {
            if (auto address = WindowProcedureAddress())
                wndProcHook = safetyhook::create_inline(address, CustomWndProc);
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
