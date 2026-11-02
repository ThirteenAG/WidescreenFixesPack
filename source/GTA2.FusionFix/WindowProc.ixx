module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/GTAClassic/InputDevices.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module WindowProc;

import ComVars;
import Window;
import Camera;
import Input;
import Options;

// The game window's messages: key rebinding, window mode, zoom keys and the input devices
export namespace GTA2WindowProc
{
    WNDPROC wndProcOld = NULL;

    LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        InputDevices::KeyboardMessage(uMsg, wParam);
        // Rebinding a key in the options page.
        int pressed = uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN ? int(wParam) : uMsg == WM_LBUTTONDOWN ? VK_LBUTTON :
            uMsg == WM_RBUTTONDOWN ? VK_RBUTTON : uMsg == WM_MBUTTONDOWN ? VK_MBUTTON :
            uMsg == WM_XBUTTONDOWN ? (GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2) : 0;
        if (pressed && GTA2Options::KeyDown(pressed)) return 0;
        switch (uMsg)
        {
            case GTA2Window::ApplyWindowModeMessage:
                GTA2Window::ApplyWindowMode(hwnd);
                return 0;
            case WM_SYSKEYDOWN:
                if (wParam == VK_RETURN && (lParam & (1u << 29)))
                {
                    if (!(lParam & (1u << 30)))
                        GTA2Options::SaveInt("MAIN", "WindowMode", windowMode == 1 ? 2 : 1);
                    return 0;
                }
                break;
            case WM_KEYDOWN:
                GTA2Input::QueueMenuKey(wParam);
                GTA2Camera::KeyDown(wParam);
                if (wParam == nZoomIncreaseKey) {
                    fZoom += 1.0f;
                }
                else if (wParam == nZoomDecreaseKey) {
                    fZoom -= 1.0f;
                }

                fZoom = std::max(0.0f, std::min(fZoom, 10.0f));
                break;
            case WM_KEYUP:
                break;
            case WM_KILLFOCUS:
                GTA2Input::MenuKeys = 0;
                GTA2Input::TextKey = 0;
                InputDevices::UpdateRumble(true);
                break;
            case WM_DESTROY:
                InputDevices::UpdateRumble(true);
                CursorConfinement::Shutdown();
                RWGTA::UpdateCheck::OnExit();
                break;
        }

        return CallWindowProc(wndProcOld, hwnd, uMsg, wParam, lParam);
    }

    HWND* pendingWindow = nullptr;
    HHOOK windowMessages = nullptr;

    void AttachGameWindow(HWND window)
    {
        wndProcOld = reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC));
        InputDevices::Window = window;
        SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc));
    }

    LRESULT CALLBACK WindowMessage(int code, WPARAM wParam, LPARAM lParam)
    {
        if (code >= 0 && pendingWindow && *pendingWindow)
        {
            const auto message = reinterpret_cast<const MSG*>(lParam);
            if (message->hwnd == *pendingWindow)
            {
                const auto window = *pendingWindow;
                pendingWindow = nullptr;
                UnhookWindowsHookEx(windowMessages);
                windowMessages = nullptr;
                AttachGameWindow(window);
            }
        }
        return CallNextHookEx(nullptr, code, wParam, lParam);
    }

    void WatchWindow(HWND* window)
    {
        if (!window)
            return;
        if (*window)
        {
            AttachGameWindow(*window);
            return;
        }
        pendingWindow = window;
        windowMessages = SetWindowsHookExW(WH_GETMESSAGE, WindowMessage, nullptr, GetCurrentThreadId());
        if (!windowMessages)
            pendingWindow = nullptr;
    }
}

class WindowProc
{
public:
    WindowProc()
    {
        WFP::onShutdownEvent() += []()
        {
            if (GTA2WindowProc::windowMessages)
            {
                UnhookWindowsHookEx(GTA2WindowProc::windowMessages);
                GTA2WindowProc::windowMessages = nullptr;
                GTA2WindowProc::pendingWindow = nullptr;
            }
        };
    }
} WindowProc;
